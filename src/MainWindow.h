#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Compiler.h"

class EditorWidget;
class OutputWidget;
class FindDialog;
class ReplaceDialog;

class MainWindow: public QMainWindow {
 Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = 0);

 public slots:
  void loadSettings();
  void saveSettings();

 signals:
  void loaded();
  void closed();

 protected:
  void closeEvent(QCloseEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void dropEvent(QDropEvent *event);

 private slots:
  bool newFile();
  bool openFile();
  bool closeFile();
  bool saveFile();
  bool saveFileAs();
  bool exit();
  void find();
  void findNext();
  void replace();
  void goToLine();
  void selectEditorFont();
  void selectOutputFont();
  void compile();
  void setupCompiler();
  void about();
  void aboutQt();
  void compiled(int);
  void updateWindowTitle();

 private:
  bool isSafeToClose();

  void readFile(QString fileName);
  void writeFile(QString fileName);

 private:
  QString fileName_;
  Compiler compiler_;
  EditorWidget *editorWidget_;
  OutputWidget *outputWidget_;
  FindDialog *lastFind_;
  ReplaceDialog *lastReplace_;
};

#endif // MAINWINDOW_H
