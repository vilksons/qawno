#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QMessageBox>
#include <QRegExp>
#include <QSettings>

#include "Compiler.h"
#include "CompilerOptionsDialog.h"
#include "EditorWidget.h"
#include "FindDialog.h"
#include "FindReplaceDialog.h"
#include "GoToDialog.h"
#include "MainWindow.h"
#include "MenuBar.h"
#include "OutputWidget.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	m_editor = new EditorWidget(this);

	setCentralWidget(m_editor);
	connect(m_editor, SIGNAL(textChanged()), SLOT(updateWindowTitle()));

	MenuBar *menuBar = new MenuBar(this);
	setMenuBar(menuBar);
	connect(menuBar->actions().fileNew, SIGNAL(triggered()), this, SLOT(newFile()));
	connect(menuBar->actions().fileOpen, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(menuBar->actions().fileClose, SIGNAL(triggered()), this, SLOT(closeFile()));
	connect(menuBar->actions().fileSave, SIGNAL(triggered()), this, SLOT(saveFile()));
	connect(menuBar->actions().fileSaveAs, SIGNAL(triggered()), this, SLOT(saveFileAs()));
	connect(menuBar->actions().fileExit, SIGNAL(triggered()), this, SLOT(exit()));
	connect(menuBar->actions().editUndo, SIGNAL(triggered()), m_editor, SLOT(undo()));
	connect(menuBar->actions().editRedo, SIGNAL(triggered()), m_editor, SLOT(redo()));
	connect(menuBar->actions().editCut, SIGNAL(triggered()), m_editor, SLOT(cut()));
	connect(menuBar->actions().editCopy, SIGNAL(triggered()), m_editor, SLOT(copy()));
	connect(menuBar->actions().editPaste, SIGNAL(triggered()), m_editor, SLOT(paste()));
	connect(menuBar->actions().editFind, SIGNAL(triggered()), this, SLOT(find()));
	connect(menuBar->actions().editFindReplace, SIGNAL(triggered()), this, SLOT(findReplace()));
	connect(menuBar->actions().editFindNext, SIGNAL(triggered()), this, SLOT(findNext()));
	connect(menuBar->actions().editFindPrev, SIGNAL(triggered()), this, SLOT(findPrev()));
	connect(menuBar->actions().editGoToLine, SIGNAL(triggered()), this, SLOT(goToLine()));
	connect(menuBar->actions().buildCompile, SIGNAL(triggered()), this, SLOT(compile()));
	connect(menuBar->actions().optionsFontEditor, SIGNAL(triggered()), SLOT(selectEditorFont()));
	connect(menuBar->actions().optionsFontOutput, SIGNAL(triggered()), SLOT(selectOutputFont()));
	connect(menuBar->actions().optionsCompiler, SIGNAL(triggered()), SLOT(setupCompiler()));
	connect(menuBar->actions().helpAboutQt, SIGNAL(triggered()), SLOT(aboutQt()));

	QDockWidget *outputDock = new QDockWidget(tr("Output"), this);
	outputDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
	m_outputWidget = new OutputWidget(this);
	m_outputWidget->setReadOnly(true);
	outputDock->setWidget(m_outputWidget);
	addDockWidget(Qt::BottomDockWidgetArea, outputDock);

	m_compiler = new Compiler(this);
	connect(m_compiler, SIGNAL(finished(int)), this, SLOT(compiled(int)));

	// Restore window state
	readSettings();

	// Open file specified at command line, if any
	if (QApplication::instance()->argc() > 1) {
		readFile(QApplication::instance()->argv()[1]);
	}
}

bool MainWindow::newFile()
{
	return closeFile();
}

bool MainWindow::openFile()
{
	if (closeFile()) {
		QFileDialog openDialog(this);
		QString fileName = openDialog.getOpenFileName(this,
			tr("Open file"), "", tr("Pawn scripts (*.pwn *.inc)"));
		readFile(fileName);
		return true;
	}
	return false;
}

bool MainWindow::isSafeToClose()
{
	if (m_editor->document()->isModified()
		&& !m_editor->document()->isEmpty())
	{
		QString message;

		if (!m_fileName.isEmpty()) {
			message = tr("Save changes to %1?").arg(m_fileName);
		} else {
			message = tr("Save changes to a new file?");
		}

		int button = QMessageBox::question(this, QCoreApplication::applicationName(),
			message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		switch (button) {
		case QMessageBox::Yes:
			saveFile();
			if (m_editor->document()->isModified()) {
				// Save file failed or cancelled
				return false;
			}
			return true;
		case QMessageBox::No:
			break;
		case QMessageBox::Cancel:
			return false;
		}
	}

	return true;
}

bool MainWindow::closeFile()
{
	if (isSafeToClose()) {
		m_editor->clear();
		m_fileName.clear();
		return true;
	}
	return false;
}

bool MainWindow::saveFile()
{
	if (m_editor->document()->isEmpty()) {
		return false;
	}

	if (m_fileName.isEmpty()) {
		saveFileAs();
		return false;
	}

	writeFile(m_fileName);
	return true;
}

bool MainWindow::saveFileAs()
{
	if (m_editor->document()->isEmpty()) {
		return false;
	}

	QFileDialog saveDialog;
	QString fileName = saveDialog.getSaveFileName(this,
		tr("Save file as"), "", tr("Pawn scripts (*.pwn *.inc)"));

	if (!fileName.isEmpty()) {
		m_fileName = fileName;
		return saveFile();
	}

	return false;
}

bool MainWindow::exit()
{
	if (closeFile()) {
		QApplication::exit();
		return true;
	}
	return false;
}

void MainWindow::find()
{
	FindDialog dialog;
	dialog.exec();

	QTextDocument::FindFlags flags;
	if (dialog.matchCase()) {
		flags |= QTextDocument::FindCaseSensitively;
	}
	if (dialog.matchWholeWords()) {
		flags |= QTextDocument::FindWholeWords;
	}
	if (dialog.searchBackwards()) {
		flags |= QTextDocument::FindBackward;
	}

	if (dialog.useRegexp()) {
		QRegExp regexp(dialog.findWhatText(),
			dialog.matchCase() ? Qt::CaseSensitive : Qt::CaseInsensitive);
		QTextCursor cur = m_editor->document()->find(regexp, m_editor->textCursor(), flags);
		m_editor->setTextCursor(cur);
	} else {
		QTextCursor cur = m_editor->document()->find(dialog.findWhatText(), m_editor->textCursor(), flags);
		m_editor->setTextCursor(cur);
	}
}

void MainWindow::findReplace()
{
	FindReplaceDialog dialog;
	dialog.exec();
}

void MainWindow::findNext()
{
}

void MainWindow::findPrev()
{
}

void MainWindow::goToLine()
{
	GoToDialog dialog;
	dialog.exec();
	m_editor->setCurrentLine(dialog.getEnteredNumber());
}

void MainWindow::selectEditorFont()
{
	QFontDialog fontDialog(this);

	bool ok = false;
	QFont newFont = fontDialog.getFont(&ok, m_editor->font(), this,
		tr("Select editor font"));
	if (ok) {
		m_editor->setFont(newFont);
	}
}

void MainWindow::selectOutputFont()
{
	QFontDialog fontDialog(this);

	bool ok = false;
	QFont newFont = fontDialog.getFont(&ok, m_outputWidget->font(), this,
		tr("Select output font"));

	if (ok) {
		m_outputWidget->setFont(newFont);
	}
}

void MainWindow::compile()
{
	if (!m_compiler->test()) {
		int button = QMessageBox::warning(this, QCoreApplication::applicationName(),
			tr("Pawn compiler is not set or missing.\n"
			   "Do you want to set compiler path now?"),
			QMessageBox::Yes | QMessageBox::No);
		if (button != QMessageBox::No) {
			setupCompiler();
		}
		return;
	}

	if (m_editor->toPlainText().isEmpty()) {
		m_outputWidget->appendPlainText(tr("Nothing to compile!"));
		return;
	}

	if (m_fileName.isEmpty()) {
		saveFileAs();
		return;
	}

	m_compiler->run(m_fileName);
}

void MainWindow::compiled(int exitCode)
{
	m_outputWidget->clear();

	QString command = m_compiler->getCommandLine(m_fileName);
	m_outputWidget->appendPlainText(command);
	m_outputWidget->appendPlainText("\n");

	QString output = m_compiler->getOutput();
	m_outputWidget->appendPlainText(output);
}

void MainWindow::setupCompiler()
{
	CompilerOptionsDialog dialog;

	dialog.setCompilerPath(m_compiler->path());
	dialog.setCompilerOptions(m_compiler->options().join(" "));

	dialog.exec();

	if (dialog.result() == QDialog::Accepted) {
		m_compiler->setPath(dialog.getCompilerPath());
		m_compiler->setOptions(dialog.getCompilerOptions());
	}
}

void MainWindow::aboutQt()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::updateWindowTitle()
{
	QString title;
	if (!m_fileName.isEmpty()) {
		title.append(QFileInfo(m_fileName).fileName());
		if (m_editor->document()->isModified()) {
			title.append("*");
		}
		title.append(" - ");
	}
	title.append(QCoreApplication::applicationName());
	setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
	if (isSafeToClose()) {
		writeSettings();
		closeEvent->accept();
	} else {
		closeEvent->ignore();
	}
}

void MainWindow::readFile(QString fileName)
{
	if (!fileName.isEmpty()) {
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::critical(this, QCoreApplication::applicationName(),
				tr("Could not open %1: %2.")
					.arg(fileName)
					.arg(file.errorString()),
				QMessageBox::Ok);
		} else {
			m_fileName = fileName;
			m_editor->setPlainText(file.readAll());
			m_editor->document()->setModified(false);
			updateWindowTitle();
		}
	}
}

void MainWindow::writeFile(QString fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::critical(this, QCoreApplication::applicationName(),
			tr("Could not save to %1: %2.")
				.arg(fileName)
				.arg(file.errorString()),
			QMessageBox::Ok);
		return;
	}

	file.write(m_editor->toPlainText().toAscii());
	m_editor->document()->setModified(false);
	updateWindowTitle();
}

void MainWindow::readSettings()
{
	QSettings settings;

	settings.beginGroup("Widgets");
		settings.beginGroup("MainWindow");
			resize(settings.value("Size", QSize(640, 480)).toSize());
			move(settings.value("Pos").toPoint());
			if (settings.value("Maximized", false).toBool()) {
				setWindowState(Qt::WindowMaximized);
			}
		settings.endGroup();
	settings.endGroup();
}

void MainWindow::writeSettings()
{
	QSettings settings;

	settings.beginGroup("Widgets");
		settings.beginGroup("MainWindow");
			settings.setValue("Maximized", isMaximized());
			if (!isMaximized()) {
				settings.setValue("Size", size());
				settings.setValue("Pos", pos());
			}
		settings.endGroup();
	settings.endGroup();
}
