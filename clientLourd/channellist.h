#ifndef CHANNELLIST_H
#define CHANNELLIST_H

#include <QDialog>
#include <QString>
#include <QTableWidget>

namespace Ui {
class Channellist;
}

class Channellist : public QDialog
{
    Q_OBJECT

public:
    //Constructor and destructor
    explicit Channellist(QWidget *parent = 0);
    ~Channellist();

    //Ui update functions
    void addRow(QString channel);
    void clear();
private slots:

private:
    Ui::Channellist *ui;
    QTableWidget *table;
};

#endif // CHANNELLIST_H