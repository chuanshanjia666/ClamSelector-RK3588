#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "rgascalerplus.h"

RGAScalerPlus::RGAScalerPlus(int out_w, int out_h, AVPixelFormat out_fmt,
                           AVBufferRef *hw_device_ctx)
    : BaseScaler(0, 0, AV_PIX_FMT_DRM_PRIME, {1, 1}, out_w, out_h, out_fmt),
      hw_device_ctx_(av_buffer_ref(hw_device_ctx)) {}

RGAScalerPlus::~RGAScalerPlus() {
    release_rga();
    av_buffer_unref(&hw_device_ctx_);
}

bool RGAScalerPlus::Init() {
    if (out_pix_fmt != AV_PIX_FMT_RGB24 && out_pix_fmt != AV_PIX_FMT_NV12) {
        std::cerr << "RGA only supports NV12/RGB24 output format" << std::endl;
        return false;
    }

    if (!init_rga()) {
        std::cerr << "Failed to initialize RGA" << std::endl;
        return false;
    }

    dst_frame_ = av_frame_alloc();
    if (!dst_frame_) {
        std::cerr << "Failed to allocate output frame" << std::endl;
        return false;
    }

    dst_frame_->format = out_pix_fmt;
    dst_frame_->width = out_width;
    dst_frame_->height = out_height;
    
    if (av_frame_get_buffer(dst_frame_, 0) < 0) {
        std::cerr << "Failed to allocate buffer for output frame" << std::endl;
        av_frame_free(&dst_frame_);
        return false;
    }

    return true;
}

bool RGAScalerPlus::init_rga() {
    if (RgaInit(&rga_ctx_) != 0) {
        std::cerr << "Failed to initialize RGA context" << std::endl;
        return false;
    }
    return true;
}

AVFrame *RGAScalerPlus::scale_and_convert(AVFrame *hw_frame) {
    if (!hw_frame || hw_frame->format != AV_PIX_FMT_DRM_PRIME) {
        std::cerr << "Invalid input frame format" << std::endl;
        return nullptr;
    }

    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)hw_frame->data[0];
    if (!desc || desc->nb_objects < 1) {
        std::cerr << "Invalid DRM frame descriptor" << std::endl;
        return nullptr;
    }

    int fd = desc->objects[0].fd;
    if (fd < 0) {
        std::cerr << "Invalid file descriptor" << std::endl;
        return nullptr;
    }

    // 配置RGA参数
    rga_info_t src_info, dst_info;
    memset(&src_info, 0, sizeof(rga_info_t));
    memset(&dst_info, 0, sizeof(rga_info_t));

    // 输入配置
    src_info.fd = fd;
    src_info.mmuFlag = 1;
    src_info.rotation = 0;
    
    // 关键修改1：设置正确的输入格式
    src_info.format = RK_FORMAT_YCbCr_420_SP; // NV12格式
    
    // 关键修改2：正确设置输入矩形区域
    rga_set_rect(&src_info.rect, 
                0, 0, 
                hw_frame->width, hw_frame->height,
                hw_frame->width, hw_frame->height,
                RK_FORMAT_YCbCr_420_SP);

    // 输出配置
    dst_info.fd = -1; // 使用内存输出
    dst_info.virAddr = dst_frame_->data[0];
    dst_info.mmuFlag = 1;
    
    // 关键修改3：设置正确的输出格式
    dst_info.format = (out_pix_fmt == AV_PIX_FMT_NV12) ? 
                     RK_FORMAT_YCbCr_420_SP : RK_FORMAT_RGB_888;
    
    // 关键修改4：正确设置输出矩形区域
    rga_set_rect(&dst_info.rect,
                0, 0,
                out_width, out_height,
                out_width, out_height,
                dst_info.format);

    // 关键修改5：添加调试信息
    // std::cout << "RGA Conversion Parameters:" << std::endl;
    // std::cout << "Input: " << hw_frame->width << "x" << hw_frame->height 
    //           << " (format: NV12)" << std::endl;
    // std::cout << "Output: " << out_width << "x" << out_height 
    //           << " (format: " << (out_pix_fmt == AV_PIX_FMT_NV12 ? "NV12" : "RGB24") 
    //           << ")" << std::endl;

    // 执行转换
    int ret = RgaBlit(&src_info, &dst_info, nullptr);
    if (ret != 0) {
        std::cerr << "RGA conversion failed with error: " << ret << std::endl;
        return nullptr;
    }

    // 复制帧属性
    dst_frame_->pts = hw_frame->pts;
    dst_frame_->pkt_dts = hw_frame->pkt_dts;
    av_frame_copy_props(dst_frame_, hw_frame);

    return av_frame_clone(dst_frame_);
}

void RGAScalerPlus::release_rga() {
    if (rga_ctx_) {
        RgaDeInit(rga_ctx_);
        rga_ctx_ = nullptr;
    }
    
    if (dst_frame_) {
        av_frame_free(&dst_frame_);
    }
}
