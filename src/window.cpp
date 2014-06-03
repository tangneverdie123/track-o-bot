#include <QtGui>
#include "window.h"

#include "ui_window.h"
#include "ui_settings_widget.h"
#include "ui_log_widget.h"
#include "ui_about_widget.h"

#include "updater.h"
extern Updater *updater;

SettingsTab::SettingsTab(QWidget *parent)
  : QWidget(parent), ui(new Ui::SettingsWidget)
{
  ui->setupUi(this);
  connect(ui->exportAccountButton, SIGNAL(clicked()), this, SLOT(exportAccount()));
  connect(ui->importAccountButton, SIGNAL(clicked()), this, SLOT(importAccount()));
  connect(ui->startAtLogin, SIGNAL(clicked(bool)), this, SLOT(applySettings()));
  connect(ui->checkForUpdates, SIGNAL(clicked(bool)), this, SLOT(applySettings()));
  connect(Tracker::Instance(), SIGNAL(AccountCreated()), this, SLOT(loadSettings()));
  loadSettings();
}

SettingsTab::~SettingsTab() {
  delete ui;
}

void SettingsTab::exportAccount() {
  QString fileName = QFileDialog::getSaveFileName(this,
      tr("Export Track-o-Bot Account Data"), "",
      tr("Account Data (*.track-o-bot);; All Files (*)"));

  if(fileName.isEmpty()) {
    return;
  } else {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
      QMessageBox::information(this, tr("Unable to open file"),
          file.errorString());
      return;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_8);
    out << Tracker::Instance()->Username();
    out << Tracker::Instance()->Password();
    out << Tracker::Instance()->WebserviceURL();

    LOG("Account %s exported in %s", Tracker::Instance()->Username().toStdString().c_str(), fileName.toStdString().c_str());
  }
}

void SettingsTab::importAccount() {
  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Import Track-o-Bot Account Data"), "",
      tr("Account Data (*.track-o-bot);; All Files (*)"));

  if(fileName.isEmpty()) {
    return;
  } else {
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) {
      QMessageBox::information(this, tr("Unable to open file"),
          file.errorString());
      return;
    }

    QDataStream in(&file);
    QString username, password, webserviceUrl;
    in.setVersion(QDataStream::Qt_4_8);
    in >> username;
    in >> password;
    in >> webserviceUrl;

    if(!username.isEmpty() && !password.isEmpty() && !webserviceUrl.isEmpty()) {
      Tracker::Instance()->SetUsername(username);
      Tracker::Instance()->SetPassword(password);
      Tracker::Instance()->SetWebserviceURL(webserviceUrl);

      LOG("Account %s imported from %s", username.toStdString().c_str(), fileName.toStdString().c_str());

      loadSettings();
    } else {
      LOG("Import failed");
    }
  }
}

void SettingsTab::applySettings() {
  Autostart autostart;
  autostart.SetActive(ui->startAtLogin->isChecked());

  if(updater) {
    updater->setAutomaticallyChecksForUpdates(ui->checkForUpdates->isChecked());
  }
}

void SettingsTab::loadSettings() {
  Autostart autostart;
  ui->startAtLogin->setChecked(autostart.IsActive());

  if(updater) {
    ui->checkForUpdates->setChecked(updater->automaticallyChecksForUpdates());
  }

  bool accountSetUp = Tracker::Instance()->IsAccountSetUp();
  if(accountSetUp) {
    ui->account->setText(Tracker::Instance()->Username());
  }

  ui->importAccountButton->setEnabled(accountSetUp);
  ui->exportAccountButton->setEnabled(accountSetUp);
}

LogTab::LogTab(QWidget *parent)
  : QWidget(parent), ui(new Ui::LogWidget)
{
  ui->setupUi(this);

  QFont font("Monospace");
  font.setStyleHint(QFont::TypeWriter);
  ui->logText->setFont(font);

  connect(Logger::Instance(), SIGNAL(NewMessage(const string&)), this, SLOT(addLogEntry(const string&)));
}

LogTab::~LogTab() {
  delete ui;
}

void LogTab::addLogEntry(const string& msg) {
  ui->logText->moveCursor(QTextCursor::End);
  ui->logText->insertPlainText(msg.c_str());
  ui->logText->moveCursor(QTextCursor::End);
}

AboutTab::AboutTab(QWidget *parent)
  : QWidget(parent), ui(new Ui::AboutWidget)
{
  ui->setupUi(this);

  QPixmap logoImage(":/icons/logo.png");
  ui->logo->setPixmap(logoImage.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  ui->version->setText(VERSION);
}

AboutTab::~AboutTab() {
  delete ui;
}

Window::Window()
  :ui(new Ui::Window)
{
  ui->setupUi(this);

  setWindowTitle(qApp->applicationName());

  createActions();
  createTrayIcon();
}

Window::~Window() {
  delete ui;
}

void Window::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  ui->settingsTab->loadSettings();
}

void Window::closeEvent(QCloseEvent *event) {
  if(trayIcon->isVisible()) {
    hide();
    event->ignore();
  }
}

// prevent esc from closing the app
void Window::reject()
{
  if(trayIcon->isVisible()) {
    hide();
  } else {
    QDialog::reject();
  }
}

void Window::createActions()
{
  openProfileAction = new QAction(tr("Open Profile..."), this);
  connect(openProfileAction, SIGNAL(triggered()), this, SLOT(openProfile()));

  showAction = new QAction(tr("Settings..."), this);
  connect(showAction, SIGNAL(triggered()), this, SLOT(riseAndShine()));

  quitAction = new QAction(tr("Quit"), this);
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void Window::createTrayIcon()
{
  trayIconMenu = new QMenu(this);
  trayIconMenu->addAction(openProfileAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(showAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);

  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setContextMenu(trayIconMenu);

#if defined Q_WS_MAC
  QIcon icon = QIcon(":/icons/tray_mac.png");
  icon.addFile(":/icons/tray_mac_selected.png", QSize(), QIcon::Selected);
#elif defined Q_WS_WIN
  QIcon icon = QIcon(":/icons/tray_win.ico");
#endif

  trayIcon->setIcon(icon);
  trayIcon->show();
}

void Window::riseAndShine() {
  show();
  raise();
}

void Window::openProfile() {
  Tracker::Instance()->OpenProfile();
}
