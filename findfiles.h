#pragma once

#ifndef FINDFILES_H
#define FINDFILES_H

#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>



class FindFiles: public QWidget
{
    Q_OBJECT

public:
    FindFiles(QWidget* parent = nullptr);
private  slots:
    void browse();
    void find();
    void animateFindClick();
    void OpenFileOfItem(int row, int column);
    void contextMenu(const QPoint &pos);

private:
    QStringList findFiles(const QStringList &files, const QString &text);
    void showFiles(const QStringList &paths);
    QComboBox* createComboBox(const QString &text = QString());
    void createFilesTable();

    QComboBox *fileComboBox;
    QComboBox *textComboBox;
    QComboBox *directoryComboBox;
    QLabel *filesFoundLabel;
    QPushButton *findButton;
    QTableWidget *filesTable;

    QDir currentDir;
};

#endif // FINDFILES_H
