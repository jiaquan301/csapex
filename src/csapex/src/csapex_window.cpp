/// HEADER
#include <csapex/csapex_window.h>

/// COMPONENT
#include "ui_evaluation_window.h"
#include <csapex/bash_parser.h>
#include <csapex/command_dispatcher.h>
#include <csapex/designer.h>
#include <csapex/graph.h>
#include <csapex/stream_interceptor.h>
#include <csapex/qt_helper.hpp>
#include <csapex/designerio.h>
#include <csapex/graphio.h>
#include <csapex/drag_io.h>
#include <csapex/box.h>

/// SYSTEM
#include <iostream>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsTextItem>

using namespace csapex;

CsApexWindow::CsApexWindow(CsApexCore& core, QWidget *parent)
    : QMainWindow(parent), core(core), ui(new Ui::EvaluationWindow), init_(false)
{
    Graph::Ptr graph = Graph::root();

    ui->setupUi(this);

    QGraphicsScene* scene = new QGraphicsScene;
    QImage splash(":/apex_splash.png");
    splash = splash.scaled(splash.width()/2, splash.height() / 2);
    scene->addPixmap(QPixmap::fromImage(splash));
    ui->loading->setScene(scene);
    ui->loading->setFixedSize(splash.size());
    ui->loading->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->loading->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    designer_ = new Designer;
    designer_->hide();
    ui->splitter->addWidget(designer_);
    ui->splitter->addWidget(ui->logOutput);

    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(save()));
    QObject::connect(ui->actionSaveAs, SIGNAL(triggered()), this,  SLOT(saveAs()));
    QObject::connect(ui->actionSaveAsCopy, SIGNAL(triggered()), this,  SLOT(saveAsCopy()));
    QObject::connect(ui->actionLoad, SIGNAL(triggered()), this,  SLOT(load()));
    QObject::connect(ui->actionReload, SIGNAL(triggered()), this,  SLOT(reload()));
    QObject::connect(ui->actionReset, SIGNAL(triggered()), this,  SLOT(reset()));

    QObject::connect(ui->actionGrid, SIGNAL(toggled(bool)), designer_,  SLOT(enableGrid(bool)));

    QObject::connect(ui->actionUndo, SIGNAL(triggered()), graph.get(),  SLOT(undo()));
    QObject::connect(ui->actionRedo, SIGNAL(triggered()), graph.get(),  SLOT(redo()));
    QObject::connect(ui->actionClear, SIGNAL(triggered()), graph.get(),  SLOT(clear()));
    QObject::connect(ui->actionClear_selection, SIGNAL(triggered()), graph.get(),  SLOT(clearSelection()));
    QObject::connect(ui->actionSelect_all, SIGNAL(triggered()), graph.get(),  SLOT(selectAll()));

    QObject::connect(graph.get(), SIGNAL(boxAdded(Box*)), designer_, SLOT(addBox(Box*)));
    QObject::connect(graph.get(), SIGNAL(boxDeleted(Box*)), designer_, SLOT(deleteBox(Box*)));
    QObject::connect(graph.get(), SIGNAL(stateChanged()), designer_, SLOT(stateChangedEvent()));
    QObject::connect(graph.get(), SIGNAL(stateChanged()), this, SLOT(updateMenu()));

    QObject::connect(&core, SIGNAL(configChanged()), this, SLOT(updateTitle()));
    QObject::connect(&core, SIGNAL(showStatusMessage(const std::string&)), this, SLOT(showStatusMessage(const std::string&)));
    QObject::connect(&core, SIGNAL(saveSettingsRequest(YAML::Emitter&)), this, SLOT(saveSettings(YAML::Emitter&)));
    QObject::connect(&core, SIGNAL(loadSettingsRequest(YAML::Node&)), this, SLOT(loadSettings(YAML::Node&)));

    QObject::connect(&CommandDispatcher::instance(), SIGNAL(dirtyChanged(bool)), this, SLOT(updateTitle()));

    QObject::connect(this, SIGNAL(initialize()), this, SLOT(init()), Qt::QueuedConnection);

    updateMenu();
    updateTitle();

    hideLog();

    timer.setInterval(100);
    timer.setSingleShot(false);
    timer.start();

    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));
}

void CsApexWindow::showMenu()
{
    QVBoxLayout* new_layout = new QVBoxLayout;

    QToolBar* tb = new QToolBar;
    QMenuBar* mb = menuBar();
    tb->addActions(mb->actions());

    new_layout->addWidget(tb);

    QLayout* layout = ui->centralwidget->layout();
    QLayoutItem* item;
    while((item = layout->takeAt(0)) != NULL) {
        new_layout->addItem(item);
    }

    delete layout;
    ui->centralwidget->setLayout(new_layout);
}

void CsApexWindow::start()
{
    showStatusMessage("initialized");

    ui->splitter->hide();

    resize(250,120);

    show();
}

void CsApexWindow::updateMenu()
{
    Graph::Ptr graph_ = Graph::root();
    ui->actionUndo->setDisabled(!graph_->canUndo());
    ui->actionRedo->setDisabled(!graph_->canRedo());
}

void CsApexWindow::updateTitle()
{
    std::stringstream window;
    window << "CS::APEX (" << core.getConfig() << ")";

    if(CommandDispatcher::instance().isDirty()) {
        window << " *";
    }

    setWindowTitle(window.str().c_str());
}

void CsApexWindow::scrollDownLog()
{
    QTextCursor cursor = ui->logOutput->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->logOutput->setTextCursor(cursor);
}

void CsApexWindow::tick()
{
    CommandDispatcher::executeLater();

    std::string latest_cout = StreamInterceptor::instance().getCout().c_str();
    std::string latest_cerr = StreamInterceptor::instance().getCerr().c_str();

    if(!latest_cout.empty()) {
        scrollDownLog();

        std::stringstream latest;
        latest << latest_cout;


        std::string line;
        while (std::getline(latest, line, '\n')) {
            if(line.substr(0, 8) == "warning:") {
                line = std::string("<span style='color: #ffcc00;'><b>") + line + "</b></span>";
            }

            line = BashParser::toHtml(line);

            line += "<br />";
            ui->logOutput->insertHtml(line.c_str());
        }

    }
    if(!latest_cerr.empty()) {
        size_t i = 0;
        while((i = latest_cerr.find('\n', i)) != std::string::npos) {
            latest_cerr.replace(i, 1, "<br />");
            i += 6;
        }

        latest_cerr = std::string("<span style='color: red'><b>") + latest_cerr + "</b></span>";

        scrollDownLog();

        ui->logOutput->insertHtml(latest_cerr.c_str());

        int height = 50;

        QList<int> sizes = ui->splitter->sizes();
        sizes[0] -= height;
        sizes[1] = height;
        ui->splitter->setSizes(sizes);
    }
}

void CsApexWindow::hideLog()
{
    QList<int> sizes = ui->splitter->sizes();
    sizes[0] += sizes[1];
    sizes[1] = 0;
    ui->splitter->setSizes(sizes);
}

void CsApexWindow::closeEvent(QCloseEvent* event)
{
    Graph::Ptr graph_ = Graph::root();
    if(graph_->isDirty()) {
        int r = QMessageBox::warning(this, tr("cs::APEX"),
                                     tr("Do you want to save the layout before closing?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if(r == QMessageBox::Save) {
            std::cout << "save" << std::endl;

            save();
            event->accept();
        } else if(r == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
            return;
        }
    }

    try {
        graph_->stop();
    } catch(...) {
        std::abort();
    }

    event->accept();
}

void CsApexWindow::showStatusMessage(const std::string &msg)
{
    statusBar()->showMessage(msg.c_str());
}

void CsApexWindow::init()
{
    init_ = true;

    core.init();

    statusBar()->hide();
    ui->loading->hide();
    repaint();
    designer_->show();
    ui->splitter->show();
    hideLog();
}

void CsApexWindow::paintEvent(QPaintEvent *e)
{
    QMainWindow::paintEvent(e);

    if(!init_) {
        Q_EMIT initialize();
    }
}

void CsApexWindow::save()
{
    core.saveAs(core.getConfig());
}

void CsApexWindow::saveAs()
{
    QString filename = QFileDialog::getSaveFileName(0, "Save config", core.getConfig().c_str(), GraphIO::config_selector.c_str());

    if(!filename.isEmpty()) {
        core.saveAs(filename.toStdString());
        core.setCurrentConfig(filename.toStdString());
    }
}


void CsApexWindow::saveAsCopy()
{
    QString filename = QFileDialog::getSaveFileName(0, "Save config", core.getConfig().c_str(), GraphIO::config_selector.c_str());

    if(!filename.isEmpty()) {
        core.saveAs(filename.toStdString());
    }
}

void CsApexWindow::reload()
{
    core.load(core.getConfig());
}

void CsApexWindow::reset()
{
    int r = QMessageBox::warning(this, tr("cs::APEX"),
                                 tr("Do you really want to reset? This <b>cannot</b> be undone!"),
                                 QMessageBox::Ok | QMessageBox::Cancel);
    if(r == QMessageBox::Ok) {
        core.reset();
    }
}

void CsApexWindow::load()
{
    QString filename = QFileDialog::getOpenFileName(0, "Load config", core.getConfig().c_str(), GraphIO::config_selector.c_str());

    if(QFile(filename).exists()) {
        core.load(filename.toStdString());
    }
}

void CsApexWindow::saveSettings(YAML::Emitter &e)
{
    DesignerIO designerio(*designer_);
    designerio.saveSettings(e);
}

void CsApexWindow::loadSettings(YAML::Node &doc)
{
    DesignerIO designerio(*designer_);
    designerio.loadSettings(doc);
}
