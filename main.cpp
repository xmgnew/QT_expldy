#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include "wifelabel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Expldy");
    window.resize(800, 600);

    auto *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);

    WifeLabel *wife = new WifeLabel(&window);
    wife->setTargetSize(QSize(400, 600));   // 统一显示尺寸（你可改）
    wife->loadFromAssets();
    wife->playIdle();

    layout->addWidget(wife, 0, Qt::AlignCenter);

    window.show();
    return app.exec();
}
