#ifndef CONFIG_THEMELIST_H
#define CONFIG_THEMELIST_H

#include <QVector>

#include "theme.h"

class QFile;

class ThemeList {
public:
    static ThemeList * instance();
    static void deleteInstance();
    void change(int newIndex);
    void addTheme();
    void loadTheme();

    //Setters for current theme
    void currentIndex(int newIndex);
    void name(QString newName);
    void background(QString newBackground);
    void hour(QString newHour);
    void nick(QString newName);
    void self(QString newSelf);
    void text(QString newText);
    void gradStart(QString newGrad);
    void gradEnd(QString newGrad);

    //Getters for current theme
    QStringList names();
    int currentIndex();
    QString name();
    QString background();
    QString hour();
    QString nick();
    QString self();
    QString text();
    QString gradStart();
    QString gradEnd();

private:
    ThemeList();
    static ThemeList *aInstance;

    int aCurrentIndex;
    QVector<Theme> themes;

    void readTheme(QFile *themefile);
};

#endif // THEMELIST_H
