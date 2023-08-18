#include <QSplitter>
#include <QDebug>

class SplitterHandle : public QSplitterHandle
{
public:
    SplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent)
    {
    }

    void paintEvent(QPaintEvent *e) override
    {
        QSplitterHandle::paintEvent(e);
    }
};

/**
 * Home made splitter to be able to define a custom handle which is border with
 * "mid" colored lines.
 */
class Splitter : public QSplitter
{
public:
    Splitter(Qt::Orientation orientation, QWidget *parent)
        : QSplitter(orientation, parent)
    {
    }

protected:
    QSplitterHandle *createHandle() override
    {
        auto handle = new SplitterHandle(orientation(), this);
        qDebug() << "creating" << orientation() << handle;
        return handle;
    }
};
