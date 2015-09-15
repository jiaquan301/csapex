/// HEADER
#include <csapex/view/node/default_node_adapter.h>

/// COMPONENT
#include <csapex/view/utility/qt_helper.hpp>
#include <csapex/view/utility/qsignal_relay.h>
#include <csapex/model/node.h>
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/view/widgets/port.h>
#include <csapex/view/designer/widget_controller.h>
#include <csapex/model/node_worker.h>
#include <csapex/view/node/parameter_context_menu.h>
#include <csapex/model/node_state.h>
#include <csapex/model/graph_worker.h>
#include <csapex/view/widgets/doublespanslider.h>

/// PROJECT
#include <csapex/param/range_parameter.h>
#include <csapex/param/interval_parameter.h>
#include <csapex/param/value_parameter.h>
#include <csapex/param/set_parameter.h>
#include <csapex/param/bitset_parameter.h>
#include <csapex/param/path_parameter.h>
#include <csapex/param/trigger_parameter.h>
#include <csapex/param/color_parameter.h>
#include <csapex/param/angle_parameter.h>
#include <csapex/param/output_progress_parameter.h>

/// SYSTEM
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <iomanip>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QGroupBox>
#include <QListWidget>
#include <QApplication>
#include <QProgressBar>
#include <QDial>
#include <functional>
#include <QLabel>
#include <qxt5/qxtspanslider.h>

using namespace csapex;
using namespace boost::lambda;
namespace bll = boost::lambda;

boost::arg<1> __1;
boost::arg<2> __2;

namespace {


/// UI HANDLES
QIntSlider* makeIntSlider(QBoxLayout* layout, const std::string& name, int def, int min, int max, int step,
                          param::RangeParameterPtr range_param,
                          csapex::ContextMenuHandler *context_handler) {
    apex_assert_hard(min<=max);

    if(((def - min) / step) * step != (def - min)) {
        std::cerr << "default " << def << " is not a multiple of minimum " << min << " with a step size of " << step << std::endl;
        def = min;
        std::cerr << "set default to " << def << std::endl;
    }

    if(((max - min) / step) * step != (max - min)) {
        std::cerr << "maximum " << max << " is not a multiple of minimum " << min << " with a step size of " << step << std::endl;
        max = ((max - min) / step) * step + min;
        std::cerr << "set maximum to " << max << std::endl;
    }


    QHBoxLayout* internal_layout = new QHBoxLayout;

    QIntSlider* slider = new QIntSlider(Qt::Horizontal, step);
    slider->setIntMinimum(min);
    slider->setIntMaximum(max);
    slider->setIntValue(def);
    slider->setMinimumWidth(100);
    slider->setSingleStep(step);

    QWrapper::QSpinBoxExt* display = new QWrapper::QSpinBoxExt;
    display->setMinimum(min);
    display->setMaximum(max);
    display->setValue(def);
    display->setSingleStep(step);

    QLabel* label = new QLabel(name.c_str());
    if(context_handler) {
        label->setContextMenuPolicy(Qt::CustomContextMenu);
        context_handler->setParent(label);
        QObject::connect(label, SIGNAL(customContextMenuRequested(QPoint)), context_handler, SLOT(showContextMenu(QPoint)));
    }

    internal_layout->addWidget(label);
    internal_layout->addWidget(slider);
    internal_layout->addWidget(display);

    layout->addLayout(internal_layout);


    for(int i = 0; i < internal_layout->count(); ++i) {
        QWidget* child = internal_layout->itemAt(i)->widget();
        child->setProperty("parameter", QVariant::fromValue(static_cast<void*>(static_cast<csapex::param::Parameter*>(range_param.get()))));
    }

    QObject::connect(slider, SIGNAL(intValueChanged(int)),  display, SLOT(setValue(int)));
    QObject::connect(slider, SIGNAL(intRangeChanged(int,int)), display, SLOT(setRange(int,int)));
    QObject::connect(display, SIGNAL(valueChanged(int)), slider, SLOT(setIntValue(int)));


    return slider;
}


QDoubleSlider* makeDoubleSlider(QBoxLayout* layout, const std::string display_name,
                                param::RangeParameterPtr range_param,
                                csapex::ContextMenuHandler *context_handler)
{
    apex_assert_hard(range_param->min<double>()<=range_param->max<double>());

    QHBoxLayout* internal_layout = new QHBoxLayout;

    QDoubleSlider* slider = new QDoubleSlider(Qt::Horizontal, range_param->step<double>());
    slider->setDoubleMaximum(range_param->max<double>());
    slider->setDoubleMinimum(range_param->min<double>());
    slider->setDoubleValue(range_param->def<double>());
    slider->setMinimumWidth(100);

    // iterate until decimal value < e
    double e = 0.0001;
    int decimals = 0;
    double f = range_param->step<double>();
    while (true) {
        double decimal_val = f - (floor(f));
        if(decimal_val < e) {
            break;
        }
        f *= 10.0;
        ++decimals;
    }

    QWrapper::QDoubleSpinBoxExt* display = new  QWrapper::QDoubleSpinBoxExt;
    display->setDecimals(decimals);
    //    display->setDecimals(std::log10(1.0 / step_size) + 1);
    display->setMinimum(range_param->min<double>());
    display->setMaximum(range_param->max<double>());
    display->setValue(range_param->def<double>());
    display->setSingleStep(range_param->step<double>());

    QLabel* label = new QLabel(QString::fromStdString(display_name));
    if(context_handler) {
        label->setContextMenuPolicy(Qt::CustomContextMenu);
        context_handler->setParent(label);
        QObject::connect(label, SIGNAL(customContextMenuRequested(QPoint)), context_handler, SLOT(showContextMenu(QPoint)));
    }

    internal_layout->addWidget(label);
    internal_layout->addWidget(slider);
    internal_layout->addWidget(display);


    layout->addLayout(internal_layout);


    for(int i = 0; i < internal_layout->count(); ++i) {
        QWidget* child = internal_layout->itemAt(i)->widget();
        child->setProperty("parameter", QVariant::fromValue(static_cast<void*>(static_cast<csapex::param::Parameter*>(range_param.get()))));
    }

    QObject::connect(slider, SIGNAL(doubleValueChanged(double)), display, SLOT(setValue(double)));
    QObject::connect(slider, SIGNAL(doubleRangeChanged(double,double)), display, SLOT(setRange(double, double)));
    QObject::connect(display, SIGNAL(valueChanged(double)), slider, SLOT(setNearestDoubleValue(double)));

    return slider;
}


QxtSpanSlider* makeSpanSlider(QBoxLayout* layout, const std::string& name, int lower, int upper, int min, int max,
                              param::IntervalParameterPtr interval_param,
                              csapex::ContextMenuHandler *context_handler)
{
    apex_assert_hard(min<=max);

    QHBoxLayout* internal_layout = new QHBoxLayout;

    QxtSpanSlider* slider = new QxtSpanSlider(Qt::Horizontal);
    slider->setMinimum(min);
    slider->setMaximum(max);
    slider->setUpperValue(upper);
    slider->setLowerValue(lower);

    QWrapper::QSpinBoxExt* displayLower = new QWrapper::QSpinBoxExt;
    displayLower->setMinimum(min);
    displayLower->setMaximum(max);
    displayLower->setValue(lower);

    QWrapper::QSpinBoxExt* displayUpper = new QWrapper::QSpinBoxExt;
    displayUpper->setMinimum(min);
    displayUpper->setMaximum(max);
    displayUpper->setValue(upper);

    QLabel* label = new QLabel(name.c_str());
    if(context_handler) {
        label->setContextMenuPolicy(Qt::CustomContextMenu);
        context_handler->setParent(label);
        QObject::connect(label, SIGNAL(customContextMenuRequested(QPoint)), context_handler, SLOT(showContextMenu(QPoint)));
    }

    internal_layout->addWidget(label);
    internal_layout->addWidget(displayLower);
    internal_layout->addWidget(slider);
    internal_layout->addWidget(displayUpper);

    layout->addLayout(internal_layout);

    for(int i = 0; i < internal_layout->count(); ++i) {
        QWidget* child = internal_layout->itemAt(i)->widget();
        child->setProperty("parameter", QVariant::fromValue(static_cast<void*>(static_cast<csapex::param::Parameter*>(interval_param.get()))));
    }

    QObject::connect(slider,        SIGNAL(rangeChanged(int,int)),  displayUpper,   SLOT(setRange(int,int)));
    QObject::connect(slider,        SIGNAL(lowerValueChanged(int)), displayLower,   SLOT(setValue(int)));
    QObject::connect(slider,        SIGNAL(upperValueChanged(int)), displayUpper,   SLOT(setValue(int)));
    QObject::connect(displayLower,  SIGNAL(valueChanged(int)),      slider,         SLOT(setLowerValue(int)));
    QObject::connect(displayUpper,  SIGNAL(valueChanged(int)),      slider,         SLOT(setUpperValue(int)));

    return slider;
}


DoubleSpanSlider* makeDoubleSpanSlider(QBoxLayout *layout, const std::string &name, double lower, double upper, double min, double max, double step_size,
                                          csapex::ContextMenuHandler *context_handler)
{
    apex_assert_hard(min<=max);

    QHBoxLayout* internal_layout = new QHBoxLayout;

    DoubleSpanSlider* slider = new DoubleSpanSlider(Qt::Horizontal, step_size);
    slider->setDoubleRange(min, max);
    slider->setSpan(lower, upper);

    QWrapper::QDoubleSpinBoxExt* displayLower = new QWrapper::QDoubleSpinBoxExt;
    displayLower->setRange(min, max);
    displayLower->setValue(lower);

    QWrapper::QDoubleSpinBoxExt* displayUpper = new QWrapper::QDoubleSpinBoxExt;
    displayUpper->setRange(min, max);
    displayUpper->setValue(upper);

    QLabel* label = new QLabel(name.c_str());
    if(context_handler) {
        label->setContextMenuPolicy(Qt::CustomContextMenu);
        context_handler->setParent(label);
        QObject::connect(label, SIGNAL(customContextMenuRequested(QPoint)), context_handler, SLOT(showContextMenu(QPoint)));
    }

    internal_layout->addWidget(label);
    internal_layout->addWidget(displayLower);
    internal_layout->addWidget(slider);
    internal_layout->addWidget(displayUpper);

    layout->addLayout(internal_layout);

    QObject::connect(slider,        SIGNAL(rangeChanged(double,double)),  displayUpper,   SLOT(setRange(double,double)));
    QObject::connect(slider,        SIGNAL(rangeChanged(double,double)),  displayLower,   SLOT(setRange(double,double)));
    QObject::connect(slider,        SIGNAL(lowerValueChanged(double)), displayLower,   SLOT(setValue(double)));
    QObject::connect(slider,        SIGNAL(upperValueChanged(double)), displayUpper,   SLOT(setValue(double)));
    QObject::connect(displayLower,  SIGNAL(valueChanged(double)),      slider,         SLOT(setLowerDoubleValue(double)));
    QObject::connect(displayUpper,  SIGNAL(valueChanged(double)),      slider,         SLOT(setUpperDoubleValue(double)));

    return slider;
}

/// THREADING
void assertGuiThread()
{
    apex_assert_hard(QThread::currentThread() == QApplication::instance()->thread());
}

void assertNotGuiThread()
{
    apex_assert_hard(QThread::currentThread() != QApplication::instance()->thread());
}
}

/// BRIDGE
DefaultNodeAdapterBridge::DefaultNodeAdapterBridge(DefaultNodeAdapter *parent)
    : parent_(parent)
{
    qRegisterMetaType < Function > ("Function");

    assertGuiThread();
    apex_assert_hard(thread() == QApplication::instance()->thread());

    QObject::connect(this, SIGNAL(setupAdaptiveUiRequest()), this, SLOT(setupAdaptiveUi()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(modelCallback(Function)), this, SLOT(executeModelCallback(Function)));
}

DefaultNodeAdapterBridge::~DefaultNodeAdapterBridge()
{
    disconnect();
}

void DefaultNodeAdapterBridge::connectInGuiThread(boost::signals2::signal<void (csapex::param::Parameter *)> &signal,
                                                  std::function<void ()> cb)
{
    // cb should be executed in the gui thread
    connections.push_back(signal.connect(std::bind(&DefaultNodeAdapterBridge::modelCallback, this, cb)));
}

void DefaultNodeAdapterBridge::disconnect()
{
    for(const boost::signals2::connection& c : connections) {
        c.disconnect();
    }

    connections.clear();
}

void DefaultNodeAdapterBridge::executeModelCallback(Function cb)
{
    assertGuiThread();
    cb();
}

void DefaultNodeAdapterBridge::setupAdaptiveUi()
{
    parent_->setupAdaptiveUi();
}

void DefaultNodeAdapterBridge::enableGroup(bool enable, const std::string &group)
{
    parent_->groups_enabled[group] = enable;
    parent_->groupsboxes[group]->setProperty("hidden", !enable);
    QWidget* w = parent_->wrapper_layout_->parentWidget();
    w->setStyleSheet(w->styleSheet());
}

void DefaultNodeAdapterBridge::triggerSetupAdaptiveUiRequest()
{
    Q_EMIT setupAdaptiveUiRequest();
}


/// ADAPTER
DefaultNodeAdapter::DefaultNodeAdapter(NodeWorkerWeakPtr adaptee, WidgetController *widget_ctrl)
    : NodeAdapter(adaptee, widget_ctrl), bridge(this), wrapper_layout_(nullptr)
{
}

DefaultNodeAdapter::~DefaultNodeAdapter()
{
    clear();
}

void DefaultNodeAdapter::clear()
{
    bridge.disconnect();

    QtHelper::clearLayout(wrapper_layout_);

    for(QObject* cb : callbacks) {
        qt_helper::Call* call = dynamic_cast<qt_helper::Call*>(cb);
        if(call) {
            call->disconnect();
        }
        delete call;
        //cb->deleteLater();
    }
    callbacks.clear();
    groups.clear();
}


namespace {


QString toColorSS(const std::vector<int>& v) {
    std::stringstream ss;
    ss << "QPushButton {";
    ss << "background-color: #" << std::hex << std::setfill('0');
    ss << std::setw(2) << v[0];
    ss << std::setw(2) << v[1];
    ss << std::setw(2) << v[2];
    ss << std::dec << ";";
    ss << "}";

    return QString::fromStdString(ss.str());
}

// COLOR ////////////////////
void ui_updateColorParameter(param::ColorParameterWeakPtr color_p, QPushButton* btn)
{
    assertGuiThread();
    auto p = color_p.lock();
    if(!p) {
        return;
    }
    btn->setStyleSheet(toColorSS(p->value()));
}

void model_updateColorParameter(param::ColorParameterWeakPtr color_p)
{
    assertGuiThread();
    auto p = color_p.lock();
    if(!p) {
        return;
    }

    std::vector<int> c = p->value();
    QColor init(c[0], c[1], c[2]);
    QColor color = QColorDialog::getColor(init);
    if (color.isValid()) {
        std::vector<int> v(3);
        v[0] = color.red();
        v[1] = color.green();
        v[2] = color.blue();
        p->set(v);
    }
}

// PATH ////////////////////
void ui_updatePathParameter(param::PathParameterWeakPtr path_p, QLineEdit* path)
{
    assertGuiThread();
    auto p = path_p.lock();
    if(!p) {
        return;
    }
    path->setText(QString::fromStdString(p->as<std::string>()));
}

void model_updatePathParameter(param::PathParameterWeakPtr path_p, QLineEdit* path)
{
    assertNotGuiThread();
    auto p = path_p.lock();
    if(!p) {
        return;
    }
    auto path_str = path->text().toStdString();
    p->set<std::string>(path_str);
}

void ui_updatePathParameterDialog(param::PathParameterWeakPtr path_p)
{
    assertGuiThread();
    auto p = path_p.lock();
    if(!p) {
        return;
    }
    QString filter = QString::fromStdString(p->filter());
    if(filter.isEmpty()) {
        filter = "All files (*.*)";
    }

    int flags = QFileDialog::DontUseNativeDialog;
    bool is_file = p->isFile();

    QString dir(p->as<std::string>().c_str());
    if(dir.startsWith("file://", Qt::CaseInsensitive)) {
        dir = dir.replace("file://", "", Qt::CaseInsensitive);
    }

    QString path;
    if(p->isOutput()) {
        if(is_file) {
            path = QFileDialog::getSaveFileName((QWidget*) 0, p->name().c_str(), dir, filter, (QString*) 0, (QFlags<QFileDialog::Option>) flags);
        } else {
            path = QFileDialog::getExistingDirectory((QWidget*) 0, p->name().c_str(), dir, (QFlags<QFileDialog::Option>) flags);
        }
    } else {
        if(is_file) {
            path = QFileDialog::getOpenFileName((QWidget*) 0, p->name().c_str(), dir, filter, (QString*) 0, (QFlags<QFileDialog::Option>) flags);
        } else {
            path = QFileDialog::getExistingDirectory((QWidget*) 0, p->name().c_str(), dir, (QFlags<QFileDialog::Option>) flags);
        }
    }

    if(!path.isEmpty()) {
        p->set(path.toStdString());
        //        cb();
    }
}

// VALUE ////////////////////
void ui_updateStringValueParameter(param::ValueParameterWeakPtr value_p, QLineEdit* txt)
{
    assertGuiThread();
    auto p = value_p.lock();
    if(!p) {
        return;
    }
    txt->setText(QString::fromStdString(p->as<std::string>()));
}
void model_updateStringValueParameter(param::ValueParameterWeakPtr value_p, QLineEdit* txt)
{
    assertNotGuiThread();
    auto p = value_p.lock();
    if(!p) {
        return;
    }
    p->set<std::string>(txt->text().toStdString());
}

void ui_updateBoolValueParameter(param::ValueParameterWeakPtr value_p, QCheckBox* cb)
{
    assertGuiThread();
    auto p = value_p.lock();
    if(!p) {
        return;
    }
    cb->setChecked(p->as<bool>());
}
void model_updateBoolValueParameter(param::ValueParameterWeakPtr value_p, QCheckBox* cb)
{
    assertNotGuiThread();
    auto p = value_p.lock();
    if(!p) {
        return;
    }
    p->set<bool>(cb->isChecked());
}

template <typename T, typename Widget>
void ui_updateValueParameter(param::ValueParameterWeakPtr value_p, Widget* box)
{
    assertGuiThread();
    auto p = value_p.lock();
    if(!p) {
        return;
    }
    box->setValue(p->as<T>());
}
template <typename T, typename Widget>
void model_updateValueParameter(param::ValueParameterWeakPtr value_p, Widget* box)
{
    assertNotGuiThread();
    auto p = value_p.lock();
    if(!p) {
        return;
    }
    p->set<T>(box->value());
}

// RANGE ////////////////////
void ui_updateIntRangeParameter(param::RangeParameterWeakPtr range_p, QIntSlider* slider)
{
    assertGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }
    slider->setIntValue(p->as<int>());
}
void model_updateIntRangeParameter(param::RangeParameterWeakPtr range_p, QIntSlider* slider)
{
    assertNotGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }
    p->set<int>(slider->intValue());
}

void ui_updateIntRangeParameterScope(param::RangeParameterWeakPtr range_p, QIntSlider* slider)
{
    assertGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }
    slider->setIntRange(p->min<int>(), p->max<int>());
}

void ui_updateDoubleRangeParameter(param::RangeParameterWeakPtr range_p, QDoubleSlider* slider)
{
    assertGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }
    slider->setDoubleValue(p->as<double>());
}
void model_updateDoubleRangeParameter(param::RangeParameterWeakPtr range_p, QDoubleSlider* slider)
{
    assertNotGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }
    p->set<double>(slider->doubleValue());
}

void ui_updateDoubleRangeParameterScope(param::RangeParameterWeakPtr range_p, QDoubleSlider* slider)
{
    assertGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }
    slider->setDoubleRange(p->min<double>(), p->max<double>());
}

// INTERVAL ////////////////////
template <typename T, typename Widget>
void ui_updateIntervalParameter(param::IntervalParameterWeakPtr interval_p, Widget* slider)
{
    assertGuiThread();
    auto p = interval_p.lock();
    if(!p) {
        return;
    }
    slider->setSpan(p->lower<T>(), p->upper<T>());
}

void model_updateIntIntervalParameter(param::IntervalParameterWeakPtr interval_p, QxtSpanSlider* slider)
{
    assertNotGuiThread();
    auto p = interval_p.lock();
    if(!p) {
        return;
    }
    p->set(std::make_pair(slider->lowerValue(), slider->upperValue()));
}
void model_updateDoubleIntervalParameter(param::IntervalParameterWeakPtr interval_p, DoubleSpanSlider* slider)
{
    assertNotGuiThread();
    auto p = interval_p.lock();
    if(!p) {
        return;
    }
    p->set(std::make_pair(slider->lowerDoubleValue(), slider->upperDoubleValue()));
}

template <typename T, typename Widget>
void ui_updateIntervalParameterScope(param::IntervalParameterWeakPtr interval_p, Widget* slider)
{
    assertGuiThread();
    auto p = interval_p.lock();
    if(!p) {
        return;
    }
    slider->setRange(p->min<T>(), p->max<T>());
}

// SET ////////////////////
void ui_updateSetParameter(param::SetParameterWeakPtr set_p, QComboBox* combo)
{
    assertGuiThread();
    auto p = set_p.lock();
    if(!p) {
        return;
    }
    int index = combo->findData(QString::fromStdString(p->getText()));
    if(index >= 0) {
        combo->setCurrentIndex(index);
    }
}
void model_updateSetParameter(param::SetParameterWeakPtr set_p, QComboBox* combo)
{
    auto p = set_p.lock();
    if(!p) {
        return;
    }
    if(!combo->currentText().isEmpty()) {
        assertNotGuiThread();
        p->setByName(combo->currentText().toStdString());
    }
}

void ui_updateSetParameterScope(param::SetParameterWeakPtr set_p, QComboBox* combo)
{
    assertGuiThread();
    auto p = set_p.lock();
    if(!p) {
        return;
    }

    int current = 0;
    combo->clear();
    std::string selected;
    try {
        selected = p->getText();
    } catch(const std::exception& e) {
        selected = "";
    }

    for(int i = 0; i < p->noParameters(); ++i) {
        std::string str = p->getText(i);
        combo->addItem(QString::fromStdString(str));

        if(str == selected) {
            current = i;
        }
    }
    combo->setCurrentIndex(current);
}

// BITSET ////////////////////
void ui_updateBitSetParameter(param::BitSetParameterWeakPtr bitset_p, QCheckBox* item, const std::string& str)
{
    assertGuiThread();
    auto p = bitset_p.lock();
    if(!p) {
        return;
    }
    item->setChecked(p->isSet(str));
}
void model_updateBitSetParameter(param::BitSetParameterWeakPtr bitset_p, QCheckBox* item, const std::string& str)
{
    assertNotGuiThread();
    auto p = bitset_p.lock();
    if(!p) {
        return;
    }
    p->setBitTo(str, item->isChecked());
}

// ANGLE ///////////////////////

double normalizeAngle(double a)
{
    double r = a;
    while(r < -M_PI) r += 2*M_PI;
    while(r >= M_PI) r -= 2*M_PI;
    return r;
}

double angleToDial(double angle)
{
    return (normalizeAngle(angle) + M_PI) * 4.0 * 180.0 / M_PI;
}

double dialToAngle(double dial)
{
    return normalizeAngle(dial  / 4.0 / 180.0 * M_PI - M_PI);
}

void model_updateAngleDialParameter(param::AngleParameterWeakPtr angle_p, QDial* dial, QDoubleSpinBox* spin)
{
    assertNotGuiThread();
    auto p = angle_p.lock();
    if(!p) {
        return;
    }

    double angle = dialToAngle(dial->value());
    double min = p->min();
    double max = p->max();

    if(angle < min) {
        angle = min;
    } else if(angle > max) {
        angle = max;
    }

    p->set<double>(angle);
    spin->blockSignals(true);
    spin->setValue(angle);
    spin->blockSignals(false);
}
void model_updateAngleSpinParameter(param::AngleParameterWeakPtr angle_p, QDial* dial, QDoubleSpinBox* spin)
{
    assertNotGuiThread();
    auto p = angle_p.lock();
    if(!p) {
        return;
    }

    double angle = spin->value();
    double min = p->min();
    double max = p->max();

    if(angle < min) {
        angle = min;
    } else if(angle > max) {
        angle = max;
    }

    p->set<double>(angle);
    dial->blockSignals(true);
    dial->setValue(angleToDial(p->as<double>()));
    dial->blockSignals(false);
}
void ui_updateAngleParameter(param::AngleParameterWeakPtr range_p, QDial* dial, QDoubleSpinBox* spin)
{
    assertGuiThread();
    auto p = range_p.lock();
    if(!p) {
        return;
    }

    double angle = normalizeAngle(p->as<double>());
    double min = p->min();
    double max = p->max();

    if(angle < min) {
        angle = min;
    } else if(angle > max) {
        angle = max;
    }
    double val = angleToDial(angle);

    dial->setValue(val);
    //spin->setValue(angle);
}

// PROGRESS ////////////////////
void ui_updateProgressParameter(param::OutputProgressParameterWeakPtr progress_p, QProgressBar* bar)
{
    assertGuiThread();
    auto p = progress_p.lock();
    if(!p) {
        return;
    }
    bar->setValue(p->getProgress());
}
void ui_updateProgressScope(param::OutputProgressParameterWeakPtr progress_p, QProgressBar* bar)
{
    assertGuiThread();
    auto p = progress_p.lock();
    if(!p) {
        return;
    }
    bar->setMaximum(p->getProgressMaximum());
}

}


void DefaultNodeAdapter::setupUi(QBoxLayout * outer_layout)
{
    if(!wrapper_layout_) {
        wrapper_layout_ = new QVBoxLayout;
        outer_layout->addLayout(wrapper_layout_);
    }

    setupAdaptiveUi();
}

namespace {
void setTooltip(QLayout* l, const QString& tooltip)
{
    for(int i = 0; i < l->count(); ++i) {
        QLayoutItem* o = l->itemAt(i);
        QWidgetItem* w = dynamic_cast<QWidgetItem*>(o);
        if(w) {
            w->widget()->setToolTip(tooltip);
        } else {
            QLayout* l = dynamic_cast<QLayout*>(o);
            if(l) {
                setTooltip(l, tooltip);
            }
        }
    }
}

void setDirection(QBoxLayout* layout, NodeWorkerWeakPtr node)
{

    NodeWorkerPtr n = node.lock();
    if(n) {
        layout->setDirection(n->getNodeState()->isFlipped() ? QHBoxLayout::RightToLeft : QHBoxLayout::LeftToRight);
    }
}

template <typename P>
void install(std::map<int, std::function<void(DefaultNodeAdapter*, csapex::param::Parameter::Ptr)> >& map)
{
    map[P().ID()] = [](DefaultNodeAdapter* a, csapex::param::Parameter::Ptr p) { a->setupParameter(std::dynamic_pointer_cast<P>(p)); };
}

}

void DefaultNodeAdapter::setupAdaptiveUi()
{
    NodeWorkerPtr node_worker = node_.lock();
    if(!node_worker) {
        return;
    }

    auto node = node_worker->getNode().lock();
    if(!node) {
        return;
    }

    static std::map<int, std::function<void(DefaultNodeAdapter*, csapex::param::Parameter::Ptr)> > mapping_;
    if(mapping_.empty()) {
        install<param::TriggerParameter>(mapping_);
        install<param::ColorParameter>(mapping_);
        install<param::PathParameter>(mapping_);
        install<param::ValueParameter>(mapping_);
        install<param::RangeParameter>(mapping_);
        install<param::IntervalParameter>(mapping_);
        install<param::SetParameter>(mapping_);
        install<param::BitSetParameter>(mapping_);
        install<param::AngleParameter>(mapping_);

        install<param::OutputProgressParameter>(mapping_);
    }

    clear();

    current_layout_ = wrapper_layout_;

    std::vector<csapex::param::Parameter::Ptr> params = node->getParameters();

    GenericState::Ptr state = std::dynamic_pointer_cast<GenericState>(node->getParameterState());
    if(state) {
        state->parameter_set_changed->disconnect_all_slots();
        state->parameter_set_changed->connect(std::bind(&DefaultNodeAdapterBridge::triggerSetupAdaptiveUiRequest, &bridge));
    }

    for(csapex::param::Parameter::Ptr p : params) {
        csapex::param::Parameter* parameter = p.get();

        if(!parameter->isEnabled()) {
            continue;
        }

        current_name_= parameter->name();
        current_display_name_ = current_name_;
        std::size_t separator_pos = current_name_.find_first_of('/');

        QBoxLayout* group_layout = nullptr;

        if(separator_pos != std::string::npos) {
            std::string group = current_name_.substr(0, separator_pos);
            current_display_name_ = current_name_.substr(separator_pos+1);

            if(groups.find(group) != groups.end()) {
                group_layout = groups[group];
            } else {
                bool hidden = group.size() > 0 && group.at(0) == '~';

                QGroupBox* gb;
                if(hidden) {
                    gb = new QGroupBox(QString::fromStdString(group.substr(1)));
                } else {
                    gb = new QGroupBox(QString::fromStdString(group));
                }

                groupsboxes[group] = gb;

                if(groups_enabled.find(group) != groups_enabled.end()) {
                    hidden = !groups_enabled[group];
                }

                gb->setContentsMargins(0,0,0,0);

                QVBoxLayout* gb_layout = new QVBoxLayout;
                gb->setLayout(gb_layout);
                gb->setCheckable(true);
                gb->setChecked(!hidden);
                gb_layout->setContentsMargins(0,0,0,0);

                group_layout = new QVBoxLayout;
                groups.insert(std::make_pair(group, group_layout));
                group_layout->setContentsMargins(0,0,0,0);

                QFrame* hider = new QFrame;
                hider->setLayout(group_layout);
                hider->setContentsMargins(0,0,0,0);
                gb_layout->addWidget(hider);

                wrapper_layout_->addWidget(gb);

                hider->setHidden(hidden);


                qt_helper::Call* call_trigger = new qt_helper::Call(std::bind(&DefaultNodeAdapterBridge::enableGroup, &bridge, std::bind(&QGroupBox::isChecked, gb), group));
                callbacks.push_back(call_trigger);
                QObject::connect(gb, SIGNAL(toggled(bool)), call_trigger, SLOT(call()));

                QObject::connect(gb, SIGNAL(toggled(bool)), hider, SLOT(setVisible(bool)));
            }
        }

        current_layout_ = new QHBoxLayout;
        setDirection(current_layout_, node_);
        node_worker->getNodeState()->flipped_changed->connect(std::bind(&setDirection, current_layout_, node_));

        // connect parameter input, if available
        InputPtr param_in = node_worker->getParameterInput(current_name_).lock();
        if(param_in) {
            Port* port = widget_ctrl_->createPort(param_in, widget_ctrl_->getBox(node_worker->getUUID()), current_layout_);

            auto pos = parameter_connections_.find(param_in.get());
            if(pos != parameter_connections_.end()) {
                pos->second.disconnect();
            }

            port->setVisible(p->isInteractive());
            parameter_connections_[param_in.get()] = p->interactive_changed.connect([port](csapex::param::Parameter*, bool i) { return port->setVisible(i); });
        }

        // generate UI element
        if(mapping_.find(p->ID()) != mapping_.end()) {
            mapping_[p->ID()](this, p);

        } else {
            current_layout_->addWidget(new QLabel((current_name_ + "'s type is not yet registered (value: " + type2name(p->type()) + ")").c_str()));
        }

        // connect parameter output, if available
        OutputPtr param_out = node_worker->getParameterOutput(current_name_).lock();
        if(param_out) {
            Port* port = widget_ctrl_->createPort(param_out, widget_ctrl_->getBox(node_worker->getUUID()), current_layout_);

            auto pos = parameter_connections_.find(param_out.get());
            if(pos != parameter_connections_.end()) {
                pos->second.disconnect();
            }

            port->setVisible(p->isInteractive());
            parameter_connections_[param_out.get()] = p->interactive_changed.connect([port](csapex::param::Parameter*, bool i) { return port->setVisible(i); });
        }

        QString tooltip = QString::fromStdString(p->description().toString());
        if(!tooltip.isEmpty()){
            setTooltip(current_layout_, tooltip);
        }

        // put into layout
        if(group_layout) {
            group_layout->addLayout(current_layout_);
        } else {
            wrapper_layout_->addLayout(current_layout_);
        }
    }
}

qt_helper::Call * DefaultNodeAdapter::makeModelCall(std::function<void()> cb)
{
    qt_helper::Call* call = new qt_helper::Call([this, cb](){
        NodeWorkerPtr node = node_.lock();
        if(node) {
            node->executionRequested(cb);
        }
    });
    callbacks.push_back(call);
    return call;
}

qt_helper::Call * DefaultNodeAdapter::makeUiCall(std::function<void()> cb)
{
    qt_helper::Call* call = new qt_helper::Call(cb);
    callbacks.push_back(call);
    return call;
}

qt_helper::Call * DefaultNodeAdapter::makePausedUiCall(std::function<void()> cb)
{
    qt_helper::Call* call = new qt_helper::Call([this, cb](){
        auto g = widget_ctrl_->getGraph();
        bool paused = g->isPaused();
        g->pauseRequest(true);
        cb();
        g->pauseRequest(paused);
    });
    callbacks.push_back(call);
    return call;
}

void DefaultNodeAdapter::setupParameter(param::TriggerParameterPtr trigger_p)
{
    QPushButton* btn = new QPushButton(trigger_p->name().c_str());

    QHBoxLayout* sub = new QHBoxLayout;
    sub->addWidget(btn);
    current_layout_->addLayout(QtHelper::wrap(current_display_name_, sub, new ParameterContextMenu(trigger_p)));

    qt_helper::Call* call_trigger = makeModelCall(std::bind(&param::TriggerParameter::trigger, trigger_p));
    QObject::connect(btn, SIGNAL(clicked()), call_trigger, SLOT(call()));
}

void DefaultNodeAdapter::setupParameter(param::ColorParameterPtr color_p)
{
    QPushButton* btn = new QPushButton;

    btn->setStyleSheet(toColorSS(color_p->value()));

    QHBoxLayout* sub = new QHBoxLayout;
    sub->addWidget(btn);
    current_layout_->addLayout(QtHelper::wrap(current_display_name_, sub, new ParameterContextMenu(color_p)));

    // ui callback
    qt_helper::Call* call = makeUiCall(std::bind(&model_updateColorParameter, color_p));
    QObject::connect(btn, SIGNAL(clicked()), call, SLOT(call()));

    // model change -> ui
    bridge.connectInGuiThread(color_p->parameter_changed, std::bind(&ui_updateColorParameter, color_p, btn));
}

void DefaultNodeAdapter::setupParameter(param::PathParameterPtr path_p)
{
    QLineEdit* path = new QLineEdit(path_p->as<std::string>().c_str());
    QPushButton* select = new QPushButton("select");

    QHBoxLayout* sub = new QHBoxLayout;

    sub->addWidget(path);
    sub->addWidget(select);

    current_layout_->addLayout(QtHelper::wrap(current_display_name_, sub, new ParameterContextMenu(path_p)));

    // ui change -> model
    qt_helper::Call* call_set_path = makeModelCall(std::bind(&model_updatePathParameter, path_p, path));
    QObject::connect(path, SIGNAL(returnPressed()), call_set_path, SLOT(call()));

    qt_helper::Call* call_select = makePausedUiCall(std::bind(&ui_updatePathParameterDialog, path_p));
    QObject::connect(select, SIGNAL(clicked()), call_select, SLOT(call()));

    // model change -> ui
    bridge.connectInGuiThread(path_p->parameter_changed, std::bind(&ui_updatePathParameter, path_p, path));
}

void DefaultNodeAdapter::setupParameter(param::ValueParameterPtr value_p)
{
    if(value_p->is<std::string>()) {
        QLineEdit* txt = new QLineEdit;
        txt->setText(value_p->as<std::string>().c_str());
        QPushButton* send = new QPushButton("set");

        QHBoxLayout* sub = new QHBoxLayout;

        sub->addWidget(txt);
        sub->addWidget(send);

        current_layout_->addLayout(QtHelper::wrap(current_display_name_, sub, new ParameterContextMenu(value_p)));

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateStringValueParameter, value_p, txt));
        QObject::connect(txt, SIGNAL(returnPressed()), call, SLOT(call()));
        QObject::connect(send, SIGNAL(clicked()), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(value_p->parameter_changed, std::bind(&ui_updateStringValueParameter, value_p, txt));

    } else if(value_p->is<bool>()) {
        QCheckBox* box = new QCheckBox;
        box->setChecked(value_p->as<bool>());

        current_layout_->addLayout(QtHelper::wrap(current_display_name_, box, new ParameterContextMenu(value_p)));

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateBoolValueParameter, value_p, box));
        QObject::connect(box, SIGNAL(toggled(bool)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(value_p->parameter_changed, std::bind(&ui_updateBoolValueParameter, value_p, box));


    } else if(value_p->is<double>()) {
        QDoubleSpinBox* box = new QDoubleSpinBox;
        box->setDecimals(10);
        box->setMaximum(1e12);
        box->setMinimum(-1e12);
        box->setValue(value_p->as<double>());

        current_layout_->addLayout(QtHelper::wrap(current_display_name_, box, new ParameterContextMenu(value_p)));

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateValueParameter<double, QDoubleSpinBox>, value_p, box));
        QObject::connect(box, SIGNAL(valueChanged(double)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(value_p->parameter_changed, std::bind(&ui_updateValueParameter<double, QDoubleSpinBox>, value_p, box));

    }  else if(value_p->is<int>()) {
        QSpinBox* box = new QSpinBox;
        box->setMaximum(std::numeric_limits<int>::max());
        box->setMinimum(std::numeric_limits<int>::min());
        box->setValue(value_p->as<int>());

        current_layout_->addLayout(QtHelper::wrap(current_display_name_, box, new ParameterContextMenu(value_p)));

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateValueParameter<int, QSpinBox>, value_p, box));
        QObject::connect(box, SIGNAL(valueChanged(int)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(value_p->parameter_changed, std::bind(&ui_updateValueParameter<int, QSpinBox>, value_p, box));

    } else {
        current_layout_->addWidget(new QLabel((current_name_ + "'s type is not yet implemented (value: " + type2name(value_p->type()) + ")").c_str()));
    }
}

void DefaultNodeAdapter::setupParameter(param::RangeParameterPtr range_p)
{
    if(range_p->is<int>()) {
        QIntSlider* slider = makeIntSlider(current_layout_, current_display_name_ ,
                                           range_p->def<int>(), range_p->min<int>(), range_p->max<int>(), range_p->step<int>(),
                                           range_p,
                                           new ParameterContextMenu(range_p));
        slider->setIntValue(range_p->as<int>());

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateIntRangeParameter, range_p, slider));
        QObject::connect(slider, SIGNAL(intValueChanged(int)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(range_p->parameter_changed, std::bind(&ui_updateIntRangeParameter, range_p, slider));

        // parameter scope changed -> update slider interval
        bridge.connectInGuiThread(range_p->scope_changed, std::bind(&ui_updateIntRangeParameterScope, range_p, slider));

    } else if(range_p->is<double>()) {
        QDoubleSlider* slider = makeDoubleSlider(current_layout_, current_display_name_ , range_p, new ParameterContextMenu(range_p));
        slider->setDoubleValue(range_p->as<double>());

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateDoubleRangeParameter, range_p, slider));
        QObject::connect(slider, SIGNAL(doubleValueChanged(double)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(range_p->parameter_changed, std::bind(&ui_updateDoubleRangeParameter, range_p, slider));

        // parameter scope changed -> update slider interval
        bridge.connectInGuiThread(range_p->scope_changed, std::bind(&ui_updateDoubleRangeParameterScope, range_p, slider));

    } else {
        current_layout_->addWidget(new QLabel((current_name_ + "'s type is not yet implemented (range: " + type2name(range_p->type()) + ")").c_str()));
    }
}

void DefaultNodeAdapter::setupParameter(param::IntervalParameterPtr interval_p)
{
    if(interval_p->is<std::pair<int, int> >()) {
        const std::pair<int,int>& v = interval_p->as<std::pair<int,int> >();
        QxtSpanSlider* slider = makeSpanSlider(current_layout_, current_display_name_,
                                               v.first, v.second, interval_p->min<int>(), interval_p->max<int>(),
                                               interval_p,
                                               new ParameterContextMenu(interval_p));

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateIntIntervalParameter, interval_p, slider));
        QObject::connect(slider, SIGNAL(lowerValueChanged(int)), call, SLOT(call()));
        QObject::connect(slider, SIGNAL(upperValueChanged(int)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(interval_p->parameter_changed, std::bind(&ui_updateIntervalParameter<int, QxtSpanSlider>, interval_p, slider));

        // parameter scope changed -> update slider interval
        bridge.connectInGuiThread(interval_p->scope_changed, std::bind(&ui_updateIntervalParameterScope<int, QxtSpanSlider>, interval_p, slider));

    } else if(interval_p->is<std::pair<double, double> >()) {
        const std::pair<double,double>& v = interval_p->as<std::pair<double,double> >();
        DoubleSpanSlider* slider = makeDoubleSpanSlider(current_layout_, current_display_name_,
                                                           v.first, v.second, interval_p->min<double>(), interval_p->max<double>(), interval_p->step<double>(),
                                                           new ParameterContextMenu(interval_p));

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateDoubleIntervalParameter, interval_p, slider));
        QObject::connect(slider, SIGNAL(lowerValueChanged(int)), call, SLOT(call()));
        QObject::connect(slider, SIGNAL(upperValueChanged(int)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(interval_p->parameter_changed, std::bind(&ui_updateIntervalParameter<double, DoubleSpanSlider>, interval_p, slider));

        // parameter scope changed -> update slider interval
        bridge.connectInGuiThread(interval_p->scope_changed, std::bind(&ui_updateIntervalParameterScope<double, DoubleSpanSlider>, interval_p, slider));

    } else {
        current_layout_->addWidget(new QLabel((current_name_ + "'s type is not yet implemented (inverval: " + type2name(interval_p->type()) + ")").c_str()));
    }
}

void DefaultNodeAdapter::setupParameter(param::SetParameterPtr set_p)
{
    QComboBox* combo = new QComboBox;

    ui_updateSetParameterScope(set_p, combo);
    current_layout_->addLayout(QtHelper::wrap(current_display_name_, combo, new ParameterContextMenu(set_p)));

    // ui change -> model
    qt_helper::Call* call = makeModelCall(std::bind(&model_updateSetParameter, set_p, combo));
    QObject::connect(combo, SIGNAL(currentIndexChanged(QString)), call, SLOT(call()));

    // model change -> ui
    bridge.connectInGuiThread(set_p->parameter_changed, std::bind(&ui_updateSetParameter, set_p, combo));

    // parameter scope changed -> update slider interval
    bridge.connectInGuiThread(set_p->scope_changed, std::bind(&ui_updateSetParameterScope, set_p, combo));

}

void DefaultNodeAdapter::setupParameter(param::BitSetParameterPtr bitset_p)
{
    QGroupBox* group = new QGroupBox(current_name_.c_str());
    QVBoxLayout* l = new QVBoxLayout;
    group->setLayout(l);

    for(int i = 0; i < bitset_p->noParameters(); ++i) {
        std::string str = bitset_p->getName(i);
        QCheckBox* item = new QCheckBox(QString::fromStdString(str));
        l->addWidget(item);
        if(bitset_p->isSet(str)) {
            item->setChecked(true);
        }

        // ui change -> model
        qt_helper::Call* call = makeModelCall(std::bind(&model_updateBitSetParameter, bitset_p, item, str));
        QObject::connect(item, SIGNAL(toggled(bool)), call, SLOT(call()));

        // model change -> ui
        bridge.connectInGuiThread(bitset_p->parameter_changed, std::bind(&ui_updateBitSetParameter, bitset_p, item, str));
    }

    current_layout_->addWidget(group);
}


void DefaultNodeAdapter::setupParameter(param::AngleParameterPtr angle_p)
{
    QLabel* label = new QLabel(angle_p->name().c_str());

    ParameterContextMenu* context_handler = new ParameterContextMenu(angle_p);
    label->setContextMenuPolicy(Qt::CustomContextMenu);
    context_handler->setParent(label);
    QObject::connect(label, SIGNAL(customContextMenuRequested(QPoint)), context_handler, SLOT(showContextMenu(QPoint)));

    current_layout_->addWidget(label);

    QDial* dial = new QDial;
    dial->setMinimum(0);
    dial->setMaximum(360.0 * 4);
    dial->setWrapping(true);
    dial->setValue(angleToDial(angle_p->as<double>()));

    current_layout_->addWidget(dial);

    QDoubleSpinBox* spin = new QDoubleSpinBox;
    spin->setValue(angle_p->as<double>());
    spin->setMinimum(-M_PI);
    spin->setMaximum(M_PI);
    spin->setDecimals(4);
    spin->setSingleStep(0.01);

    current_layout_->addWidget(spin);

    // ui change -> model
    qt_helper::Call* call_dial = makeModelCall(std::bind(&model_updateAngleDialParameter, angle_p, dial, spin));
    QObject::connect(dial, SIGNAL(valueChanged(int)), call_dial, SLOT(call()));

    qt_helper::Call* call_spin = makeModelCall(std::bind(&model_updateAngleSpinParameter, angle_p, dial, spin));
    QObject::connect(spin, SIGNAL(valueChanged(double)), call_spin, SLOT(call()));

    // model change -> ui
    bridge.connectInGuiThread(angle_p->parameter_changed, std::bind(&ui_updateAngleParameter, angle_p, dial, spin));
}

void DefaultNodeAdapter::setupParameter(param::OutputProgressParameterPtr progress)
{
    QProgressBar* bar = new QProgressBar;
    bar->setValue(progress->getProgress());
    bar->setMaximum(progress->getProgressMaximum());
    bar->setFormat(QString::fromStdString(progress->name()) + ": %p%");
    current_layout_->addWidget(bar);

    // model change -> ui
    bridge.connectInGuiThread(progress->parameter_changed, std::bind(&ui_updateProgressParameter, progress, bar));

    // parameter scope changed -> update slider interval
    bridge.connectInGuiThread(progress->scope_changed, std::bind(&ui_updateProgressScope, progress, bar));
}


void DefaultNodeAdapter::stop()
{
    NodeAdapter::stop();
    bridge.disconnect();
}
/// MOC
#include "../../../include/csapex/view/node/moc_default_node_adapter.cpp"
