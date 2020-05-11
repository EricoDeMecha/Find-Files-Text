#include "findfiles.h"

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QProgressDialog>
#include <QShortcut>
#include <QDebug>
#include <QHeaderView>
#include <QDesktopServices>
#include <QMenu>
#include <QGuiApplication>
#include <QClipboard>

enum { absoluteFileNameRole = Qt::UserRole + 1};

FindFiles::FindFiles(QWidget *parent): QWidget(parent)
{
    setWindowTitle(tr("Find files and text in files"));
    QPushButton *browseButton = new QPushButton(tr("&Browse..."),this);
    connect(browseButton, &QAbstractButton::clicked, this, &FindFiles::browse);
    findButton = new QPushButton(tr("&Find"), this);
    connect(findButton, &QAbstractButton::clicked, this , &FindFiles::find);

    fileComboBox = createComboBox(tr("*"));
    connect(fileComboBox->lineEdit(), &QLineEdit::returnPressed,this, &FindFiles::animateFindClick);
    textComboBox= createComboBox();
    connect(textComboBox->lineEdit(), &QLineEdit::returnPressed, this, &FindFiles::animateFindClick);
    directoryComboBox = createComboBox(QDir::toNativeSeparators(QDir::currentPath()));
    connect(directoryComboBox->lineEdit(), &QLineEdit::returnPressed, this, &FindFiles::animateFindClick);
    extensionsComboBox = createComboBox(tr("cpp"));
    connect(extensionsComboBox->lineEdit(), &QLineEdit::returnPressed, this, &FindFiles::animateFindClick);

    filesFoundLabel = new QLabel;
    createFilesTable();

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(new QLabel(tr("Filename: ")),0,0);
    mainLayout->addWidget(fileComboBox,0,1,1,2);
    mainLayout->addWidget(new QLabel(tr("Containing text: ")), 1,0);
    mainLayout->addWidget(textComboBox,1,1,1,2);
    mainLayout->addWidget(new QLabel("With extension e.g pdf"), 2,0);
    mainLayout->addWidget(extensionsComboBox, 2,1);
    mainLayout->addWidget(new QLabel(tr("In directory: ")),3,0);
    mainLayout->addWidget(directoryComboBox,3,1);
    mainLayout->addWidget(browseButton,3,2);
    mainLayout->addWidget(filesTable,4,0,1,3);
    mainLayout->addWidget(filesFoundLabel,5,0,1,2);
    mainLayout->addWidget(findButton,5,2);

    connect(new QShortcut(QKeySequence::Quit,this), &QShortcut::activated,qApp, &QApplication::quit);
}

// user-defined function
static  inline QString fileNameOfItem(const QTableWidgetItem* item)
{
    return item->data(absoluteFileNameRole).toString();
}

static inline void openFile(const QString &fileName)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void FindFiles::browse()
{
    QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this,tr("[INFO] Navigate"),QDir::currentPath()));
    if(!directory.isEmpty())
    {
        if(directoryComboBox->findText(directory) ==  -1)
        {
            directoryComboBox->addItem(directory);
        }
        directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
    }
}

void FindFiles::find()
{
    filesTable->setRowCount(0);

    QString fileName =fileComboBox->currentText();
    QString text = textComboBox->currentText();
    QString path =  QDir::cleanPath(directoryComboBox->currentText());
    QString fileExtension = extensionsComboBox->currentText();
    currentDir = QDir(path);

    QStringList filter;
    if(!fileName.isEmpty() || !fileExtension.isEmpty())
    {
        filter << fileName;
    }
    QDirIterator it(path, filter, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    QStringList files;
    while(it.hasNext())
    {
        files << it.next();
    }
    if(!fileExtension.isEmpty())
    {
        files = findExtension(files , fileExtension);
    }
    if(!text.isEmpty())
    {
        files = findFiles(files, text);
    }
    files.sort();
    showFiles(files);
}

void FindFiles::animateFindClick()
{
    findButton->animateClick();
}


void FindFiles::OpenFileOfItem(int row, int /* column */)
{
    const QTableWidgetItem *item = filesTable->item(row,0);
    openFile(fileNameOfItem(item));
}

void FindFiles::contextMenu(const QPoint &pos)
{
    const QTableWidgetItem *item = filesTable->itemAt(pos);
    if(!item)
        return;
    QMenu  menu;
#ifndef QT_NO_CLIPBOARD
    QAction *copyAction = menu.addAction("Copy Name");
#endif
    QAction *openAction = menu.addAction("Open");
    QAction *action = menu.exec(filesTable->mapToGlobal(pos));
    if(!action)
        return;
    const QString fileName = fileNameOfItem(item);
    if(action == openAction)
        openFile(fileName);
#ifndef QT_NO_CLIPBOARD
    else if(action == copyAction)
        QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(fileName));
#endif
}

QStringList FindFiles::findFiles(const QStringList &files, const QString &text)
{
    QProgressDialog progressDialog(this);

    progressDialog.setCancelButtonText(tr("Cancel"));
    progressDialog.setRange(0, files.size());
    progressDialog.setWindowTitle(tr("Find Files"));

    QMimeDatabase mimeDatabase;
    QStringList foundFiles;

    for(int i = 0; i < files.size(); ++i)
    {
        progressDialog.setValue(i);
        progressDialog.setLabelText(tr("Searching file number %1 of %n...", nullptr, files.size()).arg(i));
        QCoreApplication::processEvents();

        const  QString fileName = files.at(i);
        const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileName);

        if(mimeType.isValid() && !mimeType.inherits(QStringLiteral("text/plain")))
        {
            qWarning() << "Not searching binary file "<< QDir::toNativeSeparators(fileName);
            continue;
        }
        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly))
        {
            QString line;
            QTextStream in(&file);
            while(!in.atEnd())
            {
                if(progressDialog.wasCanceled())
                {
                    break;
                }
                line  = in.readLine();
                if(line.contains(text, Qt::CaseInsensitive))
                {
                    foundFiles << files[i];
                    break;
                }
            }
        }
    }
    return foundFiles;
}

QStringList FindFiles::findExtension(const QStringList &files, const QString &extension)
{
    QProgressDialog progressDialog(this);

    progressDialog.setCancelButtonText(tr("quit"));
    progressDialog.setRange(0, files.size());
    progressDialog.setWindowTitle(tr("Find files with ext."));

    QStringList filesFound;
    QMimeDatabase db;
    for(int i = 0; i < files.size(); ++i)
    {
        progressDialog.setValue(i);
        progressDialog.setLabelText(tr("Determining file number %1 of %n...", nullptr, files.size()).arg(i));
        QCoreApplication::processEvents();
        if(progressDialog.wasCanceled())
            break;
        const QString fileName = files.at(i);
        QMimeType  mime  = db.mimeTypeForFile(fileName);
        const QString fileExtension = mime.preferredSuffix();

        if(fileExtension == extension)
        {
            filesFound << files[i];
        }
    }
    return filesFound;
}

void FindFiles::showFiles(const QStringList &paths)
{
    for(const QString &filePath : paths)
    {
        const  QString toolTip = QDir::toNativeSeparators(filePath);
        const  QString relativePath = QDir::toNativeSeparators(currentDir.relativeFilePath(filePath));
        const qint64 size = QFileInfo(filePath).size();

        QTableWidgetItem *fileNameItem = new QTableWidgetItem(relativePath);
        fileNameItem->setData(absoluteFileNameRole, QVariant(filePath));
        fileNameItem->setToolTip(toolTip);
        fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new  QTableWidgetItem(QLocale().formattedDataSize(size));
        sizeItem->setData(absoluteFileNameRole, QVariant(filePath));
        sizeItem->setToolTip(toolTip);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

        int row = filesTable->rowCount();
        filesTable->insertRow(row);
        filesTable->setItem(row, 0, fileNameItem);
        filesTable->setItem(row, 1 , sizeItem);
    }
    filesFoundLabel->setText(tr("%n file(s) found (Double click on a file to open it)", nullptr , paths.size()));
    filesFoundLabel->setWordWrap(true);
}

QComboBox *FindFiles::createComboBox(const QString &text)
{
    QComboBox *comboBox  = new QComboBox;
    comboBox->setEditable(true);
    comboBox->addItem(text);
    comboBox->setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Preferred);
    return comboBox;
}

void FindFiles::createFilesTable()
{
    filesTable = new QTableWidget(0,2);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList labels;
    labels << tr("Filename") << tr("Size");
    filesTable->setHorizontalHeaderLabels(labels);
    filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    filesTable->verticalHeader()->hide();
    filesTable->setShowGrid(false);
    filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(filesTable, &QTableWidget::customContextMenuRequested,this, &FindFiles::contextMenu);
    connect(filesTable, &QTableWidget::cellActivated, this, &FindFiles::OpenFileOfItem);
}
