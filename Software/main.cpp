#include "mainwindow.h"
#include <QApplication>
using namespace std;
#include <QDebug>
#include "decodethread.h"
extern "C"
{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
#include <libavutil/hwcontext.h>
#include <libavfilter/avfilter.h>
}

void print_all_decoders()
{
    void *iter = nullptr;
    const AVCodec *codec = nullptr;
    printf("Available decoders:\n");
    while ((codec = av_codec_iterate(&iter)))
    {
        if (av_codec_is_decoder(codec))
        {
            printf("  %s (%s)\n", codec->name, codec->long_name ? codec->long_name : "");
        }
    }
}

void print_all_hwdevice()
{
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
    {
        qDebug() << "Supported HW:" << av_hwdevice_get_type_name(type);
    }
}

void print_all_scalers()
{
    const AVFilter *filter = nullptr;
    void *opaque = nullptr; // 用于迭代的指针

    qDebug() << "Available Scaling Filters:";
    while ((filter = av_filter_iterate(&opaque)))
    {
        // 检查滤镜名称是否包含 "scale"（不区分大小写）
        std::string filter_name(filter->name);
        std::transform(filter_name.begin(), filter_name.end(), filter_name.begin(), ::tolower);

        if (filter_name.find("scale") != std::string::npos)
        {
            qDebug() << "  - " << filter->name << " (" << (filter->description ? filter->description : "No description") << ")";
        }
    }
}

int main(int argc, char *argv[])
{
    avformat_network_init();
    print_all_hwdevice();
    print_all_scalers();
    // print_all_decoders();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    a.exec();
    return 0;
}
