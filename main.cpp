#include <QApplication>
#include <QWidget>
#include "wifelabel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Expldy");
    window.resize(800, 600);

    auto *wife = new WifeLabel(&window);
    wife->setTargetSize(QSize(400, 600));
    wife->loadFromAssets();
    wife->playIdle();

    // 初始居中（窗口内拖动）
    wife->move(
        (window.width() - wife->width()) / 2,
        (window.height() - wife->height()) / 2
    );

    window.show();
    return app.exec();
}
