#ifndef MARKOVCHAINGUI_H
#define MARKOVCHAINGUI_H

#include <cstdint>
#include <istream>
#include <QAbstractButton>
#include <QFuture>
#include <QMainWindow>
#include <QMutex>
#include <QProgressBar>
#include <string>
#include <vector>
#include "libmarkov.h"

namespace Ui {
class MarkovChainGUI;
class FileOpenException;
}

class MarkovChainGUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit MarkovChainGUI(QWidget *parent = 0);
    ~MarkovChainGUI();

private:
    void handleFatalError(QString message);
    void resetModel();
    void initModel();
    void buildModel();
    void createAnalysisInputStream();
    QString getAnalysisResult(QTime analysisTime);
    void showErrorDialog(QString message);
    void queueDirectory(QString directory);

private slots:
    void inputSourceToggled(QAbstractButton *button);
    void sourcePathBrowserRequested();
    void analysisButtonPressed();
    void resetButtonPressed();
    void randomTextButtonPressed();
    void randomTextSeparatorSelected(int index);

private:
    Ui::MarkovChainGUI *ui;
    QProgressBar *progressBar;
    MarkovChain *markovChain;
    std::istream *analysisInputStream;
    std::vector<std::string> analysisWordSeparators;
    QMutex markovChainMutex;
    QFuture<void> analysisFuture;
    std::uint32_t analysisProgress;
    std::uint32_t analyzedWordTotal;
    std::vector<std::string> inputFileQueue;
};

class FileOpenException : public std::runtime_error
{
public:
    FileOpenException();
};

#endif
