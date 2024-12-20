#include "encdec.hpp"
#include "layers.hpp"
#include "model.hpp"
#include <Eigen/Core>
#include <Eigen/Dense>
#include <cmath>

void demucscpp::apply_freq_encoder(const struct demucscpp::demucs_model &model,
                                   int encoder_idx,
                                   const Eigen::Tensor3dXf &x_in,
                                   Eigen::Tensor3dXf &x_out)
{
    Eigen::Tensor3dXf x_shuf = x_in.shuffle(Eigen::array<int, 3>({2, 0, 1}));

    // 2D Convolution operation
    Eigen::Tensor3dXf y;

    switch (encoder_idx)
    {
    case 0:
        y = demucscpp::conv1d_fused_gelu<4, 48, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    case 1:
        y = demucscpp::conv1d_fused_gelu<48, 96, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    case 2:
        y = demucscpp::conv1d_fused_gelu<96, 192, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    case 3:
        y = demucscpp::conv1d_fused_gelu<192, 384, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    };

    // reverse all dims
    Eigen::Tensor3dXf y_shuff = y.shuffle(Eigen::array<int, 3>({2, 1, 0}));
    demucscpp::apply_dconv(model, y_shuff, 0, 0, encoder_idx,
                           y_shuff.dimension(2));

    // swap back from H,C,W to C,H,W
    // then put W in front to use conv1d function for width=1 conv2d
    y = y_shuff.shuffle(Eigen::array<int, 3>({2, 1, 0}));

    // need rewrite, norm2, glu
    switch (encoder_idx)
    {
    case 0:
        y = demucscpp::conv1d<48, 96, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    case 1:
        y = demucscpp::conv1d<96, 192, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    case 2:
        y = demucscpp::conv1d<192, 384, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    case 3:
        y = demucscpp::conv1d<384, 768, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    };

    y_shuff = y.shuffle(Eigen::array<int, 3>({1, 2, 0}));

    // copy into x_out
    x_out = demucscpp::glu(y_shuff, 0);
}

void demucscpp::apply_time_encoder(const struct demucscpp::demucs_model &model,
                                   int tencoder_idx,
                                   const Eigen::Tensor3dXf &xt_in,
                                   Eigen::Tensor3dXf &xt_out)
{
    int crop = demucscpp::TIME_BRANCH_LEN_0;
    // switch case for tencoder_idx
    switch (tencoder_idx)
    {
    case 0:
        break;
    case 1:
        crop = demucscpp::TIME_BRANCH_LEN_1;
        break;
    case 2:
        crop = demucscpp::TIME_BRANCH_LEN_2;
        break;
    case 3:
        crop = demucscpp::TIME_BRANCH_LEN_3;
        break;
    }

    // now implement the forward pass
    // first, apply the convolution
    // Conv1d(2, 48, kernel_size=(8,), stride=(4,), padding=(2,))
    Eigen::Tensor3dXf yt;

    switch (tencoder_idx)
    {
    case 0:
        yt = demucscpp::conv1d_fused_gelu<2, 48, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    case 1:
        yt = demucscpp::conv1d_fused_gelu<48, 96, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    case 2:
        yt = demucscpp::conv1d_fused_gelu<96, 192, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    case 3:
        yt = demucscpp::conv1d_fused_gelu<192, 384, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    };

    // now dconv time
    demucscpp::apply_dconv(model, yt, 1, 0, tencoder_idx, crop);

    // end of dconv?

    // need rewrite, norm2, glu
    switch (tencoder_idx)
    {
    case 0:
        yt = demucscpp::conv1d<48, 96, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    case 1:
        yt = demucscpp::conv1d<96, 192, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    case 2:
        yt = demucscpp::conv1d<192, 384, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    case 3:
        yt = demucscpp::conv1d<384, 768, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    };

    xt_out = demucscpp::glu(yt, 1);
}

void demucscpp::apply_freq_decoder(const struct demucscpp::demucs_model &model,
                                   int decoder_idx,
                                   const Eigen::Tensor3dXf &x_in,
                                   Eigen::Tensor3dXf &x_out,
                                   const Eigen::Tensor3dXf &skip)
{
    Eigen::Tensor3dXf y = x_in + skip;

    // need rewrite, norm2, glu
    switch (decoder_idx)
    {
    case 0:
        y = demucscpp::conv2d<384, 768, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.decoder_rewrite_weight[decoder_idx],
            model.decoder_rewrite_bias[decoder_idx]);
        break;
    case 1:
        y = demucscpp::conv2d<192, 384, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.decoder_rewrite_weight[decoder_idx],
            model.decoder_rewrite_bias[decoder_idx]);
        break;
    case 2:
        y = demucscpp::conv2d<96, 192, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.decoder_rewrite_weight[decoder_idx],
            model.decoder_rewrite_bias[decoder_idx]);
        break;
    case 3:
        y = demucscpp::conv2d<48, 96, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.decoder_rewrite_weight[decoder_idx],
            model.decoder_rewrite_bias[decoder_idx]);
        break;
    };

    y = demucscpp::glu(y, 0);

    // swap first and second dimensions
    // from C,H,W into H,C,W
    Eigen::Tensor3dXf y_shuff = y.shuffle(Eigen::array<int, 3>({1, 0, 2}));
    y = y_shuff;

    // start the DConv
    demucscpp::apply_dconv(model, y, 0, 1, 4 - decoder_idx - 1, y.dimension(2));

    // dconv finished

    // swap back from H,C,W to C,H,W
    Eigen::Tensor3dXf y_shuff_2 = y.shuffle(Eigen::array<int, 3>({1, 0, 2}));

    // now time for the transpose convolution

    // 2D Convolution operation
    switch (decoder_idx)
    {
    case 0:
        y = demucscpp::conv2d_tr_fused_gelu<384, 192, 8, 1, 4, 1, 0, 0, 1, 1>(
            y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
            model.decoder_conv_tr_bias[decoder_idx]);
        break;
    case 1:
        y = demucscpp::conv2d_tr_fused_gelu<192, 96, 8, 1, 4, 1, 0, 0, 1, 1>(
            y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
            model.decoder_conv_tr_bias[decoder_idx]);
        break;
    case 2:
        y = demucscpp::conv2d_tr_fused_gelu<96, 48, 8, 1, 4, 1, 0, 0, 1, 1>(
            y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
            model.decoder_conv_tr_bias[decoder_idx]);
        break;
    case 3:
        if (model.num_sources == 6)
        {
            y = demucscpp::conv2d_tr<48, 24, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
        }
        else if (model.num_sources == 4)
        {
            y = demucscpp::conv2d_tr<48, 16, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
        }
        else if (model.num_sources == 2)
        {
            y = demucscpp::conv2d_tr<48, 8, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
        }
        break;
    };

    int y_dim1_begin = 2;
    int y_dim1_end = y.dimension(1) - 4;

    // remove 2 elements from begin and end of y along dimension 1 (0, 1, 2)
    x_out = y.slice(Eigen::array<Eigen::Index, 3>({0, y_dim1_begin, 0}),
                    Eigen::array<Eigen::Index, 3>(
                        {y.dimension(0), y_dim1_end, y.dimension(2)}));
}

void demucscpp::apply_time_decoder(const struct demucscpp::demucs_model &model,
                                   int tdecoder_idx,
                                   const Eigen::Tensor3dXf &xt_in,
                                   Eigen::Tensor3dXf &xt_out,
                                   const Eigen::Tensor3dXf &skip)
{
    int crop = demucscpp::TIME_BRANCH_LEN_3;
    int out_length = demucscpp::TIME_BRANCH_LEN_2;
    // switch case for tdecoder_idx
    switch (tdecoder_idx)
    {
    case 0:
        break;
    case 1:
        crop = demucscpp::TIME_BRANCH_LEN_2;
        out_length = demucscpp::TIME_BRANCH_LEN_1;
        break;
    case 2:
        crop = demucscpp::TIME_BRANCH_LEN_1;
        out_length = demucscpp::TIME_BRANCH_LEN_0;
        break;
    case 3:
        crop = demucscpp::TIME_BRANCH_LEN_0;
        out_length = demucscpp::TIME_BRANCH_LEN_IN;
        break;
    }

    // need rewrite, norm2, glu
    Eigen::Tensor3dXf yt;
    switch (tdecoder_idx)
    {
    case 0:
        yt = demucscpp::conv1d<384, 768, 3, 1, 1, 1>(
            xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
            model.tdecoder_rewrite_bias[tdecoder_idx]);
        break;
    case 1:
        yt = demucscpp::conv1d<192, 384, 3, 1, 1, 1>(
            xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
            model.tdecoder_rewrite_bias[tdecoder_idx]);
        break;
    case 2:
        yt = demucscpp::conv1d<96, 192, 3, 1, 1, 1>(
            xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
            model.tdecoder_rewrite_bias[tdecoder_idx]);
        break;
    case 3:
        yt = demucscpp::conv1d<48, 96, 3, 1, 1, 1>(
            xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
            model.tdecoder_rewrite_bias[tdecoder_idx]);
        break;
    };

    yt = demucscpp::glu(yt, 1);

    // start the DConv
    demucscpp::apply_dconv(model, yt, 1, 1, 4 - tdecoder_idx - 1, crop);

    // dconv finished

    // next, apply the final transpose convolution
    Eigen::Tensor3dXf yt_tmp;

    switch (tdecoder_idx)
    {
    case 0:
        yt_tmp = demucscpp::conv1d_tr_fused_gelu<384, 192, 8, 4, 0, 1>(
            yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
            model.tdecoder_conv_tr_bias[tdecoder_idx]);
        break;
    case 1:
        yt_tmp = demucscpp::conv1d_tr_fused_gelu<192, 96, 8, 4, 0, 1>(
            yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
            model.tdecoder_conv_tr_bias[tdecoder_idx]);
        break;
    case 2:
        yt_tmp = demucscpp::conv1d_tr_fused_gelu<96, 48, 8, 4, 0, 1>(
            yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
            model.tdecoder_conv_tr_bias[tdecoder_idx]);
        break;
    case 3:
        if (model.num_sources == 6)
        {
            yt_tmp = demucscpp::conv1d_tr<48, 12, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
        }
        else if (model.num_sources == 4)
        {
            yt_tmp = demucscpp::conv1d_tr<48, 8, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
        }
        else if (model.num_sources == 2)
        {
            yt_tmp = demucscpp::conv1d_tr<48, 4, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
        }
        break;
    };

    yt = yt_tmp;

    // remove padding
    // 2:2+length
    xt_out = yt.slice(Eigen::array<Eigen::Index, 3>({0, 0, 2}),
                      Eigen::array<Eigen::Index, 3>(
                          {yt.dimension(0), yt.dimension(1), out_length}));
}

void demucscpp_v3::apply_freq_encoder_v3(
    const struct demucscpp_v3::demucs_v3_model &model, int encoder_idx,
    const Eigen::Tensor3dXf &x_in, Eigen::Tensor3dXf &x_out)
{
    Eigen::Tensor3dXf x_shuf = x_in.shuffle(Eigen::array<int, 3>({2, 0, 1}));

    // 2D Convolution operation
    Eigen::Tensor3dXf y;

    switch (encoder_idx)
    {
    case 0:
        y = demucscpp::conv1d_fused_gelu<4, 48, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    case 1:
        y = demucscpp::conv1d_fused_gelu<48, 96, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    case 2:
        y = demucscpp::conv1d_fused_gelu<96, 192, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    case 3:
        y = demucscpp::conv1d_fused_gelu<192, 384, 8, 4, 2, 1>(
            x_shuf, model.encoder_conv_weight[encoder_idx],
            model.encoder_conv_bias[encoder_idx]);
        break;
    };

    // reverse all dims
    Eigen::Tensor3dXf y_shuff = y.shuffle(Eigen::array<int, 3>({2, 1, 0}));
    demucscpp_v3::apply_dconv_v3(model, y_shuff, 0, encoder_idx,
                                 y_shuff.dimension(2));

    // swap back from H,C,W to C,H,W
    // then put W in front to use conv1d function for width=1 conv2d
    y = y_shuff.shuffle(Eigen::array<int, 3>({2, 1, 0}));

    // need rewrite, norm2, glu
    switch (encoder_idx)
    {
    case 0:
        y = demucscpp::conv1d<48, 96, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    case 1:
        y = demucscpp::conv1d<96, 192, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    case 2:
        y = demucscpp::conv1d<192, 384, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    case 3:
        y = demucscpp::conv1d<384, 768, 1, 1, 0, 1>(
            y, model.encoder_rewrite_weight[encoder_idx],
            model.encoder_rewrite_bias[encoder_idx]);
        break;
    };

    y_shuff = y.shuffle(Eigen::array<int, 3>({1, 2, 0}));

    // copy into x_out
    x_out = demucscpp::glu(y_shuff, 0);
}

void demucscpp_v3::apply_time_encoder_v3(
    const struct demucscpp_v3::demucs_v3_model &model, int tencoder_idx,
    const Eigen::Tensor3dXf &xt_in, Eigen::Tensor3dXf &xt_out)
{
    int crop = demucscpp::TIME_BRANCH_LEN_0;
    // switch case for tencoder_idx
    switch (tencoder_idx)
    {
    case 0:
        break;
    case 1:
        crop = demucscpp::TIME_BRANCH_LEN_1;
        break;
    case 2:
        crop = demucscpp::TIME_BRANCH_LEN_2;
        break;
    case 3:
        crop = demucscpp::TIME_BRANCH_LEN_3;
        break;
    }

    // now implement the forward pass
    // first, apply the convolution
    // Conv1d(2, 48, kernel_size=(8,), stride=(4,), padding=(2,))
    Eigen::Tensor3dXf yt;

    switch (tencoder_idx)
    {
    case 0:
        yt = demucscpp::conv1d_fused_gelu<2, 48, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    case 1:
        yt = demucscpp::conv1d_fused_gelu<48, 96, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    case 2:
        yt = demucscpp::conv1d_fused_gelu<96, 192, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    case 3:
        yt = demucscpp::conv1d_fused_gelu<192, 384, 8, 4, 2, 1>(
            xt_in, model.tencoder_conv_weight[tencoder_idx],
            model.tencoder_conv_bias[tencoder_idx]);
        break;
    };

    // now dconv time
    demucscpp_v3::apply_dconv_v3(model, yt, 1, tencoder_idx, crop);

    // end of dconv?

    // need rewrite, norm2, glu
    switch (tencoder_idx)
    {
    case 0:
        yt = demucscpp::conv1d<48, 96, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    case 1:
        yt = demucscpp::conv1d<96, 192, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    case 2:
        yt = demucscpp::conv1d<192, 384, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    case 3:
        yt = demucscpp::conv1d<384, 768, 1, 1, 0, 1>(
            yt, model.tencoder_rewrite_weight[tencoder_idx],
            model.tencoder_rewrite_bias[tencoder_idx]);
        break;
    };

    xt_out = demucscpp::glu(yt, 1);
}

void demucscpp_v3::apply_time_encoder_4(
    const struct demucscpp_v3::demucs_v3_model &model,
    const Eigen::Tensor3dXf &xt_in, Eigen::Tensor3dXf &xt_out)
{
    // now implement the forward pass
    // first, apply the convolution
    // Conv1d(2, 48, kernel_size=(8,), stride=(4,), padding=(2,))
    Eigen::Tensor3dXf yt = demucscpp::conv1d<384, 768, 8, 4, 2, 1>(
        xt_in, model.tencoder_4_conv_weight, model.tencoder_4_conv_bias);

    xt_out = yt;
}

void demucscpp_v3::apply_freq_encoder_4(
    const struct demucscpp_v3::demucs_v3_model &model,
    const Eigen::Tensor3dXf &x_in, const Eigen::Tensor3dXf &x_inject,
    Eigen::Tensor3dXf &x_out,
    struct demucscpp_v3::demucs_v3_segment_buffers &buffers)
{
    const int encoder_idx = 0;

    // 2D Convolution operation
    Eigen::Tensor3dXf y;
    Eigen::Tensor3dXf y_shuff;

    y = demucscpp::conv2d<384, 768, 8, 1, 4, 1, 0, 0, 1, 1>(
        x_in, model.encoder_4_conv_weight,
        model.encoder_4_5_conv_bias[encoder_idx]);
    // encoder 4 (i.e. encoder_idx 0) has the inject param
    // swap first two dims of x_inject

    y_shuff = y.shuffle(Eigen::array<int, 3>({1, 0, 2})) + x_inject;

    // apply groupnorm
    y = groupnorm::group_norm_fused_gelu(
        y_shuff, model.encoder_4_5_norm1_weight[encoder_idx],
        model.encoder_4_5_norm1_bias[encoder_idx], 4, 1e-05);

    // special dconv with bilstm + local attn
    demucscpp_v3::apply_dconv_v3_encoder_4_5(model, y, encoder_idx,
                                             y_shuff.dimension(2), buffers);

    y = demucscpp::conv1d<768, 1536, 1, 1, 0, 1>(
        y, model.encoder_4_5_rewrite_weight[encoder_idx],
        model.encoder_4_5_rewrite_bias[encoder_idx]);

    // apply groupnorm
    y = demucscpp::group_norm(y, model.encoder_4_5_norm2_weight[encoder_idx],
                              model.encoder_4_5_norm2_bias[encoder_idx], 4,
                              1e-05);

    // copy into x_out
    y_shuff = y.shuffle(Eigen::array<int, 3>({1, 0, 2}));
    x_out = demucscpp::glu(y_shuff, 0);
}

void demucscpp_v3::apply_shared_encoder_5(
    const struct demucscpp_v3::demucs_v3_model &model,
    const Eigen::Tensor3dXf &x_in, Eigen::Tensor3dXf &x_out,
    struct demucscpp_v3::demucs_v3_segment_buffers &buffers)
{
    // 2D Convolution operation
    Eigen::Tensor3dXf y;
    Eigen::Tensor3dXf y_shuff;

    const int encoder_idx = 1;

    // shuffle first two dims of x_in
    // first assign y to x_in with first two dims swapped
    y = x_in.shuffle(Eigen::array<int, 3>({1, 0, 2}));

    y = demucscpp::conv1d<768, 1536, 4, 2, 1, 1>(
        y, model.encoder_5_conv_weight,
        model.encoder_4_5_conv_bias[encoder_idx]);

    // apply groupnorm
    y = groupnorm::group_norm_fused_gelu(
        y, model.encoder_4_5_norm1_weight[encoder_idx],
        model.encoder_4_5_norm1_bias[encoder_idx], 4, 1e-05);

    // special dconv with bilstm + local attn
    demucscpp_v3::apply_dconv_v3_encoder_4_5(model, y, encoder_idx,
                                             y.dimension(2), buffers);

    // need rewrite, norm2, glu
    y = demucscpp::conv1d<1536, 3072, 1, 1, 0, 1>(
        y, model.encoder_4_5_rewrite_weight[encoder_idx],
        model.encoder_4_5_rewrite_bias[encoder_idx]);

    // apply groupnorm
    y = demucscpp::group_norm(y, model.encoder_4_5_norm2_weight[encoder_idx],
                              model.encoder_4_5_norm2_bias[encoder_idx], 4,
                              1e-05);

    // copy into x_out
    x_out = demucscpp::glu(y, 1);
}

Eigen::Tensor3dXf demucscpp_v3::apply_shared_decoder_0(
    const struct demucscpp_v3::demucs_v3_model &model, Eigen::Tensor3dXf &x_out,
    const Eigen::Tensor3dXf &skip)
{
    const int decoder_idx = 0;

    // input is empty, so we use skip directly
    Eigen::Tensor3dXf y = skip;

    y = demucscpp::conv1d<1536, 3072, 3, 1, 1, 1>(
        y, model.decoder_0_rewrite_weight,
        model.decoder_0_1_rewrite_bias[decoder_idx]);

    // apply groupnorm1 with norm1 weights
    y = groupnorm::group_norm(y, model.decoder_0_1_norm1_weight[decoder_idx],
                              model.decoder_0_1_norm1_bias[decoder_idx], 4,
                              1e-05);

    y = demucscpp::glu(y, 1);

    // return pre, to be used optionally (as first input to time decoder)
    Eigen::Tensor3dXf pre_ret = y;

    // no dconv for decoders
    // simply conv_tr -> norm2
    y = demucscpp::conv1d_tr<1536, 768, 4, 2, 0, 1>(
        y, model.decoder_0_conv_tr_weight,
        model.decoder_0_1_conv_tr_bias[decoder_idx]);

    y = groupnorm::group_norm_fused_gelu(
        y, model.decoder_0_1_norm2_weight[decoder_idx],
        model.decoder_0_1_norm2_bias[decoder_idx], 4, 1e-05);

    // remove extra elems equivalent to `1:337`
    x_out = y.slice(Eigen::array<Eigen::Index, 3>({0, 0, 1}),
                    Eigen::array<Eigen::Index, 3>(
                        {y.dimension(0), y.dimension(1), FREQ_BRANCH_LEN}));

    return pre_ret;
}

Eigen::Tensor3dXf demucscpp_v3::apply_freq_decoder_1(
    const struct demucscpp_v3::demucs_v3_model &model,
    const Eigen::Tensor3dXf &x_in, Eigen::Tensor3dXf &x_out,
    const Eigen::Tensor3dXf &skip)
{
    const int decoder_idx = 1;

    Eigen::Tensor3dXf y = x_in.shuffle(Eigen::array<int, 3>({1, 0, 2})) + skip;

    // first glu(norm1(rewrite))
    y = demucscpp::conv2d<768, 1536, 3, 3, 1, 1, 1, 1, 1, 1>(
        y, model.decoder_1_rewrite_weight,
        model.decoder_0_1_rewrite_bias[decoder_idx]);

    // apply groupnorm1 with norm1 weights
    y = groupnorm::group_norm_2(y, model.decoder_0_1_norm1_weight[decoder_idx],
                                model.decoder_0_1_norm1_bias[decoder_idx], 4,
                                1e-05);

    y = demucscpp::glu(y, 0);

    // return pre, to be used optionally (as first input to time decoder)
    Eigen::Tensor3dXf pre_ret = y;

    // no dconv for decoders
    // simply conv_tr -> norm2

    // 2D Convolution operation
    y = demucscpp::conv2d_tr<768, 384, 8, 1, 4, 1, 0, 0, 1, 1>(
        y, model.decoder_1_conv_tr_weight,
        model.decoder_0_1_conv_tr_bias[decoder_idx]);

    y = groupnorm::group_norm_fused_gelu_2(
        y, model.decoder_0_1_norm2_weight[decoder_idx],
        model.decoder_0_1_norm2_bias[decoder_idx], 4, 1e-05);

    // no slicing for this one

    x_out = y;
    return pre_ret;
}

void demucscpp_v3::apply_time_decoder_0(
    const struct demucscpp_v3::demucs_v3_model &model,
    const Eigen::Tensor3dXf &x_in, Eigen::Tensor3dXf &x_out)
{
    // simple decoder
    // rewrite and conv_tr, no group norms
    // swap first two dims
    Eigen::Tensor3dXf y_shuff = x_in.shuffle(Eigen::array<int, 3>({1, 0, 2}));

    // no norm1, rewrite, dconv for tdecoder0
    // simply conv_tr -> norm2

    // 2D Convolution operation
    Eigen::Tensor3dXf y = demucscpp::conv1d_tr<768, 384, 8, 4, 0, 1>(
        y_shuff, model.tdecoder_0_conv_tr_weight,
        model.tdecoder_0_conv_tr_bias);

    // now apply groupnorm2 with norm2 weights
    y = groupnorm::group_norm_fused_gelu(y, model.tdecoder_0_norm2_weight,
                                         model.tdecoder_0_norm2_bias, 4, 1e-05);

    // for time branch, crop to length
    int out_length = x_out.dimension(2);
    x_out = y.slice(Eigen::array<Eigen::Index, 3>({0, 0, 2}),
                    Eigen::array<Eigen::Index, 3>(
                        {y.dimension(0), y.dimension(1), out_length}));
}

void demucscpp_v3::apply_common_decoder(
    const struct demucscpp_v3::demucs_v3_model &model,
    const int freq_or_time_idx, const int decoder_idx,
    const Eigen::Tensor3dXf &x_in, Eigen::Tensor3dXf &x_out,
    const Eigen::Tensor3dXf &skip)
{
    // simple decoder
    // rewrite and conv_tr, no group norms

    Eigen::Tensor3dXf y = x_in + skip;

    // first glu(norm1(rewrite))
    if ((freq_or_time_idx == 0) && (decoder_idx == 0))
    {
        y = demucscpp::conv2d<384, 768, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.freq_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 0))
    {
        y = demucscpp::conv1d<384, 768, 3, 1, 1, 1>(
            y, model.time_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 0) && (decoder_idx == 1))
    {
        y = demucscpp::conv2d<192, 384, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.freq_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 1))
    {
        y = demucscpp::conv1d<192, 384, 3, 1, 1, 1>(
            y, model.time_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 0) && (decoder_idx == 2))
    {
        y = demucscpp::conv2d<96, 192, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.freq_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 2))
    {
        y = demucscpp::conv1d<96, 192, 3, 1, 1, 1>(
            y, model.time_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 0) && (decoder_idx == 3))
    {
        y = demucscpp::conv2d<48, 96, 3, 3, 1, 1, 1, 1, 1, 1>(
            y, model.freq_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 3))
    {
        y = demucscpp::conv1d<48, 96, 3, 1, 1, 1>(
            y, model.time_decoders_rewrite_weight[decoder_idx],
            model.decoders_rewrite_bias[freq_or_time_idx][decoder_idx]);
    }

    y = demucscpp::glu(y, freq_or_time_idx);

    // no dconv for decoders, no norm2
    // simply conv_tr

    // 2D Convolution operation
    if ((freq_or_time_idx == 0) && (decoder_idx == 0))
    {
        y = demucscpp::conv2d_tr_fused_gelu<384, 192, 8, 1, 4, 1, 0, 0, 1, 1>(
            y, model.freq_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 0))
    {
        y = demucscpp::conv1d_tr_fused_gelu<384, 192, 8, 4, 0, 1>(
            y, model.time_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 0) && (decoder_idx == 1))
    {
        y = demucscpp::conv2d_tr_fused_gelu<192, 96, 8, 1, 4, 1, 0, 0, 1, 1>(
            y, model.freq_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 1))
    {
        y = demucscpp::conv1d_tr_fused_gelu<192, 96, 8, 4, 0, 1>(
            y, model.time_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 0) && (decoder_idx == 2))
    {
        y = demucscpp::conv2d_tr_fused_gelu<96, 48, 8, 1, 4, 1, 0, 0, 1, 1>(
            y, model.freq_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 2))
    {
        y = demucscpp::conv1d_tr_fused_gelu<96, 48, 8, 4, 0, 1>(
            y, model.time_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 0) && (decoder_idx == 3))
    {
        y = demucscpp::conv2d_tr<48, 16, 8, 1, 4, 1, 0, 0, 1, 1>(
            y, model.freq_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }
    else if ((freq_or_time_idx == 1) && (decoder_idx == 3))
    {
        y = demucscpp::conv1d_tr<48, 8, 8, 4, 0, 1>(
            y, model.time_decoders_conv_tr_weight[decoder_idx],
            model.decoders_conv_tr_bias[freq_or_time_idx][decoder_idx]);
    }

    if (freq_or_time_idx == 1)
    {
        // for time branch, crop to length
        int out_length = x_out.dimension(2);
        x_out = y.slice(Eigen::array<Eigen::Index, 3>({0, 0, 2}),
                        Eigen::array<Eigen::Index, 3>(
                            {y.dimension(0), y.dimension(1), out_length}));
    }
    else
    {
        // for freq branch
        int desired_dim1_length = x_out.dimension(1);

        // remove 2 elements from begin and end of y along dimension 1 (0, 1, 2)
        x_out =
            y.slice(Eigen::array<Eigen::Index, 3>({0, 2, 0}),
                    Eigen::array<Eigen::Index, 3>(
                        {y.dimension(0), desired_dim1_length, y.dimension(2)}));
    }
}
