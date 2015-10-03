#include <ctime>
#include <fstream>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFuture>
#include <QLocale>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QTime>
#include <sstream>
#include "markovchaingui.h"
#include "ui_markovchaingui.h"

using namespace std;

const int fatalExitCode = 1;
const int timeFloatAccuracy = 2;
const int statusUpdateInterval = 50;

FileOpenException::FileOpenException() :
    runtime_error("FileOpenException")
{
}

MarkovChainGUI::MarkovChainGUI(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MarkovChainGUI), markovChain(nullptr),
    analysisInputStream(nullptr), analysisProgress(0), analyzedWordTotal(0)
{
    ui->setupUi(this);

    ui->randomTextSeparatorText->setVisible(false);

    connect(ui->sourceSelectButtons, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(inputSourceToggled(QAbstractButton*)));
    connect(ui->fileSourceBrowseButton, SIGNAL(clicked()), this, SLOT(sourcePathBrowserRequested()));
    connect(ui->analysisButton, SIGNAL(clicked()), this, SLOT(analysisButtonPressed()));
    connect(ui->resetButton, SIGNAL(clicked()), this, SLOT(resetButtonPressed()));
    connect(ui->randomTextButton, SIGNAL(clicked()), this, SLOT(randomTextButtonPressed()));
    connect(ui->randomTextSeparatorSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(randomTextSeparatorSelected(int)));

    resetModel();
}

MarkovChainGUI::~MarkovChainGUI()
{
    delete analysisInputStream;
    delete markovChain;
    delete ui;
}

void MarkovChainGUI::handleFatalError(QString message)
{
    qDebug() << message;
    showErrorDialog(message);
    qApp->exit(fatalExitCode);
}

void MarkovChainGUI::resetModel()
{
    delete markovChain;
    markovChain = nullptr;
    ui->statusBarProgress->setVisible(false);
    ui->statusBarText->setText(tr("Model is empty."));
    ui->chainLength->setEnabled(true);
}

void MarkovChainGUI::initModel()
{
    ui->chainLength->setEnabled(false);
    delete markovChain;
    analyzedWordTotal = 0;
    markovChain = new MarkovChain(ui->chainLength->value());
    ui->generationTab->setEnabled(true);
}

void MarkovChainGUI::inputSourceToggled(QAbstractButton *button)
{
    bool fileSource = (button == ui->fileSourceButton || button == ui->directorySourceButton);
    bool textSource = (button == ui->textSourceButton);
    ui->sourceTextInput->setEnabled(textSource);
    ui->fileSourcePath->setEnabled(fileSource);
    ui->fileSourceBrowseButton->setEnabled(fileSource);
}

void MarkovChainGUI::randomTextSeparatorSelected(int index)
{
    switch(index)
    {
    case 3: // Custom character
        ui->randomTextSeparatorText->setVisible(true);
        break;
    default: // Fixed character
        ui->randomTextSeparatorText->setVisible(false);
    }
}

void MarkovChainGUI::createAnalysisInputStream()
{
    delete analysisInputStream;
    analysisInputStream = nullptr;
    if (ui->textSourceButton->isChecked())
    {
        analysisInputStream = new stringstream(ui->sourceTextInput->toPlainText().toStdString(),
                                               ios_base::in);
    }
    else if (ui->fileSourceButton->isChecked() || ui->directorySourceButton->isChecked())
    {
        string inputFile = ui->fileSourceButton->isChecked() ?
                           ui->fileSourcePath->text().toStdString() :
                           inputFileQueue.back();
        ifstream *inputStream = new ifstream(inputFile);
        if (!inputStream->is_open())
        {
            throw FileOpenException();
        }
        analysisInputStream = inputStream;
    }
    if (ui->directorySourceButton->isChecked())
    {
        inputFileQueue.pop_back();
    }
}

void MarkovChainGUI::analysisButtonPressed()
{
    uint32_t inputItemCount;
    if (ui->directorySourceButton->isChecked())
    {
        inputFileQueue.clear();
        queueDirectory(ui->fileSourcePath->text());
        if (inputFileQueue.empty())
        {
            showErrorDialog("No files found in input folder.");
            return;
        }
        inputItemCount = inputFileQueue.size();
    }
    else
    {
        inputItemCount = 1;
    }

    ui->inputTabWidget->setEnabled(false);
    ui->outputTabWidget->setEnabled(false);
    ui->statusBarProgress->reset();
    ui->statusBarProgress->setVisible(true);
    if (markovChain == nullptr)
    {
        initModel();
    }
    analysisWordSeparators.clear();
    if (ui->processingSpace->isChecked())
    {
        analysisWordSeparators.push_back(" ");
    }
    if (ui->processingNewline->isChecked())
    {
        analysisWordSeparators.push_back("\n");
    }
    if (ui->processingCarriage->isChecked())
    {
        analysisWordSeparators.push_back("\r");
    }
    if (ui->processingTabulator->isChecked())
    {
        analysisWordSeparators.push_back("\t");
    }
    if (ui->processingCustom->isChecked())
    {
        for (char &separator: ui->processingCustomChars->text().toStdString())
        {
            analysisWordSeparators.push_back(string(1, separator));
        }
    }

    QTime analysisTime;
    analysisTime.start();
    uint32_t fileOpenErrorCount = 0;
    for (uint32_t i = 0; i < inputItemCount; i++)
    {
        try
        {
            createAnalysisInputStream();
        }
        catch (FileOpenException e)
        {
            qDebug() << "FileOpenException";
            fileOpenErrorCount++;
            continue;
        }
        analysisFuture = QtConcurrent::run(this, &MarkovChainGUI::buildModel);
        while (!analysisFuture.isFinished())
        {
            if (analysisTime.elapsed() % statusUpdateInterval == 0)
            {
                ui->statusBarText->setText(getAnalysisResult(analysisTime));
            }
            ui->statusBarProgress->setValue(((double) i
                                                / inputItemCount
                                                * ui->statusBarProgress->maximum())
                                            );
            qApp->processEvents();
        }
        analyzedWordTotal += analysisProgress;
    }
    analysisProgress = 0;

    ui->statusBarProgress->setValue(ui->statusBarProgress->maximum());
    delete analysisInputStream;
    analysisInputStream = nullptr;
    ui->statusBarText->setText(getAnalysisResult(analysisTime));
    ui->statusBarText->setText(ui->statusBarText->text()
                                                  .append(" " + tr("Model contains %1 unique words.")
                                                  .arg(markovChain->getUniqueWordCount())));
    ui->inputTabWidget->setEnabled(true);
    ui->outputTabWidget->setEnabled(true);
    if (fileOpenErrorCount)
    {
        showErrorDialog(tr("Failed to open %1 file(s).").arg(fileOpenErrorCount));
    }
    ui->statusBarProgress->setVisible(false);
}

void MarkovChainGUI::resetButtonPressed()
{
    resetModel();
}

QString MarkovChainGUI::getAnalysisResult(QTime analysisTime)
{
    return tr("Processed %1 words in %2 seconds.")
            .arg(analyzedWordTotal + analysisProgress)
            .arg(QLocale::system().toString(analysisTime.elapsed() / 1000.0, 'f', timeFloatAccuracy));
}

void MarkovChainGUI::buildModel()
{
    QMutexLocker markovChainMutexLock(&markovChainMutex);
    StreamingMarkovWordSource wordSource(*analysisInputStream, analysisWordSeparators);
    if (markovChain != nullptr)
    {
        try
        {
            markovChain->feedModel(wordSource, &analysisProgress);
        }
        catch (MarkovIOException e)
        {
            qDebug() << "feedModel: IOException";
        }
        catch (MarkovMissingWordException)
        {
            qDebug() << "feedModel: MissingWordException";
        }
        catch (MarkovModelEmptyException)
        {
            qDebug() << "feedModel: ModelEmptyException";
        }
        catch (MarkovException)
        {
            qDebug() << "feedModel: Unspecified exception";
        }
    }
}

void MarkovChainGUI::sourcePathBrowserRequested()
{
    if (ui->fileSourceButton->isChecked())
    {
        ui->fileSourcePath->setText(QFileDialog::getOpenFileName(this,
                                                                 tr("Select input file"),
                                                                 QDir::currentPath(),
                                                                 tr("Text files (*.txt);;All files (*.*)")
                                                                 ));
    }
    else if (ui->directorySourceButton->isChecked())
    {
        ui->fileSourcePath->setText(QFileDialog::getExistingDirectory(this,
                                                                      tr("Select folder of input files"),
                                                                      QDir::currentPath()
                                                                      ));
    }
}

void MarkovChainGUI::randomTextButtonPressed()
{
    QMutexLocker markovChainMutexLock(&markovChainMutex);
    if (markovChain == nullptr)
    {
        handleFatalError("internal error: randomTextButtonPressed: markovChain is null");
        return;
    }
    ui->inputTabWidget->setEnabled(false);
    ui->randomTextButton->setEnabled(false);
    ui->statusBarProgress->reset();
    ui->statusBarProgress->setVisible(true);
    int wordCount = ui->randomTextWordCount->value();
    QString separator;
    switch (ui->randomTextSeparatorSelect->currentIndex())
    {
    case 0:
        separator = " ";
        break;
    case 1:
        separator = "\n";
        break;
    case 3:
        separator = ui->randomTextSeparatorText->text();
        break;
    default:
        separator = "";
    }
    bool resetOccurred = false;
    uint32_t resetCount = 0;
    QString generatedText;
    generatedText.reserve(wordCount * 3);
    for (int i = wordCount; i > 0; i--)
    {
        qApp->processEvents();
        generatedText += QString::fromStdString(markovChain->generateWord(&resetOccurred));
        if (i != 1)
        {
            generatedText += separator;
        }
        if (resetOccurred)
        {
            resetCount++;
        }
        if (i % 10 == 0)
        {
            ui->statusBarProgress->setValue(ui->statusBarProgress->maximum()
                                            - ((double) i
                                                / wordCount
                                                * ui->statusBarProgress->maximum())
                                            );
            ui->resetRateLabel->setText(tr("Dead end rate: %1%")
                                        .arg((int) round((double) resetCount / wordCount * 100)));
        }
        if (i <= 10)
        {
            ui->statusBarProgress->setValue(ui->statusBarProgress->maximum());
        }
    }
    ui->randomTextOutput->setPlainText(generatedText);
    ui->inputTabWidget->setEnabled(true);
    ui->randomTextButton->setEnabled(true);
    ui->statusBarProgress->setVisible(false);
}

void MarkovChainGUI::showErrorDialog(QString message)
{
    QMessageBox::critical(this, tr("Fatal error"), message);
}

void MarkovChainGUI::queueDirectory(QString directoryPath) {
    QDir directory(directoryPath);
    QFileInfoList files = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    foreach(const QFileInfo &file, files)
    {
        QString filePath = file.absoluteFilePath();
        if (file.isDir())
        {
            queueDirectory(filePath);
        }
        else
        {
            inputFileQueue.push_back(filePath.toStdString());
        }
        qApp->processEvents();
    }
}
