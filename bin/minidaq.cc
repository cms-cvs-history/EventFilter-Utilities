
#include <QtWebKit>
#include <QMainWindow>
#include <QtGui>


int main(int argc, char **argv)
{
  QApplication app(argc,argv,true);
  QWidget window;
  window.resize(1200,500);
  window.show();
  window.setWindowTitle(QApplication::translate("toplevel","Top-level widget"));
  QWebView *view = new QWebView(&window);
  view->load(QUrl("http://fuval-c2f12-26:40002/urn:xdaq-application:lid=27/microState"));
  view->show();
  QTextDocument(
  return app.exec();
}

//  class Previewer : public QWidget
//  {
//      Q_OBJECT

//  public:
//      Previewer(QWidget *parent = 0);

//      void setBaseUrl(const QUrl &url);

//  public slots:
//      void on_previewButton_clicked();

//  private:
//      QUrl baseUrl;
//  };

//  Previewer::Previewer(QWidget *parent)
//      : QWidget(parent)
//  {

//  }

//  void Previewer::setBaseUrl(const QUrl &url)
//  {
//      baseUrl = url;
//  }

//  void Previewer::on_previewButton_clicked()
//  {
//      // Update the contents in web viewer
//      webView->setHtml(text, baseUrl);
//  }




//  class QAction;
//  class QMenu;

//  class MainWindow : public QMainWindow
//  {
//      Q_OBJECT

//  public:
//      MainWindow();

//  private slots:
//      void open();
//      void openUrl();
//      void save();
//      void about();
//      void updateTextEdit();

//  private:
//      Previewer *centralWidget;
//      QMenu *fileMenu;
//      QMenu *helpMenu;
//      QAction *openAct;
//      QAction *openUrlAct;
//      QAction *saveAct;
//      QAction *exitAct;
//      QAction *aboutAct;
//      QAction *aboutQtAct;

//      void createActions();
//      void createMenus();
//      void setStartupText();
//  }; 




//  MainWindow::MainWindow()
//  {
//      createActions();
//      createMenus();
//      centralWidget = new Previewer(this);
//      setCentralWidget(centralWidget);

//      connect(centralWidget->webView, SIGNAL(loadFinished(bool)),
//          this, SLOT(updateTextEdit()));
//      setStartupText();
//  }

//  void MainWindow::createActions()
//  {
//      openAct = new QAction(tr("&Open..."), this);
//      openAct->setShortcuts(QKeySequence::Open);
//      openAct->setStatusTip(tr("Open an existing HTML file"));
//      connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

//      openUrlAct = new QAction(tr("&Open URL..."), this);
//      openUrlAct->setShortcut(tr("Ctrl+U"));
//      openUrlAct->setStatusTip(tr("Open a URL"));
//      connect(openUrlAct, SIGNAL(triggered()), this, SLOT(openUrl()));

//      saveAct = new QAction(tr("&Save"), this);
//      saveAct->setShortcuts(QKeySequence::Save);
//      saveAct->setStatusTip(tr("Save the HTML file to disk"));
//      connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

//      exitAct = new QAction(tr("E&xit"), this);
//      exitAct->setStatusTip(tr("Exit the application"));
//      exitAct->setShortcuts(QKeySequence::Quit);
//      connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

//      aboutAct = new QAction(tr("&About"), this);
//      aboutAct->setStatusTip(tr("Show the application's About box"));
//      connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

//      aboutQtAct = new QAction(tr("About &Qt"), this);
//      aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
//      connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
//  }

//  void MainWindow::createMenus()
//  {
//      fileMenu = menuBar()->addMenu(tr("&File"));
//      fileMenu->addAction(openAct);
//      fileMenu->addAction(openUrlAct);
//      fileMenu->addAction(saveAct);
//      fileMenu->addSeparator();
//      fileMenu->addAction(exitAct);

//      menuBar()->addSeparator();

//      helpMenu = menuBar()->addMenu(tr("&Help"));
//      helpMenu->addAction(aboutAct);
//      helpMenu->addAction(aboutQtAct);
//  }

//  void MainWindow::about()
//  {
//      QMessageBox::about(this, tr("About Previewer"),
//          tr("The <b>Previewer</b> example demonstrates how to "
//             "view HTML documents using a QWebView."));
//  }

//  void MainWindow::open()
//  {
//      QString fileName = QFileDialog::getOpenFileName(this);
//      if (!fileName.isEmpty()) {
//          // read from file
//          QFile file(fileName);

//          if (!file.open(QIODevice::ReadOnly)) {
//              QMessageBox::information(this, tr("Unable to open file"),
//                  file.errorString());
//              return;
//          }

//          QTextStream out(&file);
//          QString output = out.readAll();

//          // display contents
//          centralWidget->setBaseUrl(QUrl::fromLocalFile(fileName));
//      }
//  }

//  void MainWindow::openUrl()
//  {
//      bool ok;
//      QString url = QInputDialog::getText(this, tr("Enter a URL"),
//                    tr("URL:"), QLineEdit::Normal, "http://", &ok);

//      if (ok && !url.isEmpty()) {
//          centralWidget->webView->setUrl(url);
//      }
//  }

//  void MainWindow::save()
//  {
//      QString fileName = QFileDialog::getSaveFileName(this);

//      if (!fileName.isEmpty()) {
//          // save to file
//          QFile file(fileName);

//          if (!file.open(QIODevice::WriteOnly)) {
//              QMessageBox::information(this, tr("Unable to open file"),
//                  file.errorString());
//              return;
//          }

//          QTextStream in(&file);
//          in << content;
//      }
//  }

//  void MainWindow::updateTextEdit()
//  {
//      QWebFrame *mainFrame = centralWidget->webView->page()->mainFrame();
//      QString frameText = mainFrame->toHtml();
//  }

//  void MainWindow::setStartupText()
//  {
//      QString string = "<html><body><h1>HTML Previewer</h1>"
//                       " <p>This example shows you how to use QWebView to"
//                       " preview HTML data written in a QPlainTextEdit.</p>"
//                       " </body></html>";
//      centralWidget->webView->setHtml(string);
//  }



