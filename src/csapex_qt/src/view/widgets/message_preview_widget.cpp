/// HEADER
#include <csapex/view/widgets/message_preview_widget.h>

/// PROJECT
#include <csapex/msg/direct_connection.h>
#include <csapex/manager/message_renderer_manager.h>
#include <csapex/msg/io.h>
#include <csapex/msg/generic_value_message.hpp>
#include <csapex/msg/any_message.h>
#include <csapex/utility/uuid_provider.h>
#include <csapex/utility/exceptions.h>

/// SYSTEM
#include <QApplication>
#include <QGraphicsPixmapItem>
#include <QFocusEvent>
#include <iostream>

using namespace csapex;
using namespace csapex::impl;

PreviewInput::PreviewInput(QPointer<MessagePreviewWidget> parent) : Input(UUIDProvider::makeUUID_without_parent("message_preview_in")), parent_(parent)
{
    setType(std::make_shared<connection_types::AnyMessage>());
    setVirtual(true);
}

void PreviewInput::setToken(TokenPtr token)
{
    Input::setToken(token);

    if (isConnected()) {
        TokenDataConstPtr message = token->getTokenData();

        if (!parent_) {
            return;
        }
        try {
            if (msg::isExactValue<std::string>(this)) {
                parent_->displayTextRequest(QString::fromStdString(msg::getValue<std::string>(this)));

            } else if (msg::isExactValue<int>(this)) {
                parent_->displayTextRequest(QString::number(msg::getValue<int>(this)));

            } else if (msg::isExactValue<float>(this)) {
                parent_->displayTextRequest(QString::number(msg::getValue<float>(this)));

            } else if (msg::isExactValue<double>(this)) {
                parent_->displayTextRequest(QString::number(msg::getValue<float>(this)));

            } else {
                MessageRenderer::Ptr renderer = MessageRendererManager::instance().createMessageRenderer(message);
                if (renderer) {
                    std::unique_ptr<QImage> img = renderer->render(message);
                    if (parent_) {
                        parent_->displayImageRequest(*img);
                    }
                }
            }
        } catch (const std::exception& e) {
            // silent death
        }
    }
}

void PreviewInput::detach()
{
    parent_.clear();
}

MessagePreviewWidget::MessagePreviewWidget() : pm_item_(nullptr), txt_item_(nullptr)
{
    input_ = std::make_shared<impl::PreviewInput>(this);
    setScene(new QGraphicsScene);

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    scene()->setBackgroundBrush(QBrush());

    qRegisterMetaType<TokenDataConstPtr>("TokenDataConstPtr");

    QObject::connect(this, &MessagePreviewWidget::displayImageRequest, this, &MessagePreviewWidget::displayImage);
    QObject::connect(this, &MessagePreviewWidget::displayTextRequest, this, &MessagePreviewWidget::displayText);

    setMaximumSize(256, 256);
    setAutoFillBackground(false);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

MessagePreviewWidget::~MessagePreviewWidget()
{
    if (scene()) {
        delete scene();
        setScene(nullptr);
    }
    input_->detach();
    if (isConnected()) {
        disconnect();
    }
}

void MessagePreviewWidget::connectTo(ConnectorPtr c)
{
    scene()->clear();

    if (OutputPtr o = std::dynamic_pointer_cast<Output>(c)) {
        connectToImpl(o);
    } else if (InputPtr i = std::dynamic_pointer_cast<Input>(c)) {
        connectToImpl(i);
    } else {
        return;
    }

    if (connection_) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
}

void MessagePreviewWidget::connectToImpl(OutputPtr out)
{
    try {
        connection_ = DirectConnection::connect(out, input_);
        apex_assert_soft_msg(connection_, "could not connect preview widget");

    } catch (const csapex::Failure& e) {
        std::cerr << "connecting preview panel to " << out->getUUID() << " caused an assertion error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "connecting preview panel to " << out->getUUID() << " failed: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "connecting preview panel to " << out->getUUID() << " failed with unknown cause" << std::endl;
    }
}

void MessagePreviewWidget::connectToImpl(InputPtr in)
{
    if (in->isConnected()) {
        ConnectorPtr s = in->getSource()->shared_from_this();
        if (OutputPtr source = std::dynamic_pointer_cast<Output>(s)) {
            connection_ = DirectConnection::connect(source, input_);
        }
    }
}

void MessagePreviewWidget::disconnect()
{
    if (!connection_) {
        return;
    }

    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }

    OutputPtr out = connection_->from();
    if (out) {
        out->removeConnection(input_.get());
    }
    connection_.reset();
}

void MessagePreviewWidget::displayText(const QString& txt)
{
    if (!txt_item_ || txt != displayed_) {
        displayed_ = txt;

        if (pm_item_) {
            scene()->removeItem(pm_item_);
            delete pm_item_;
            pm_item_ = nullptr;
        }

        if (txt_item_) {
            txt_item_->setPlainText(txt);
        } else {
            txt_item_ = scene()->addText(displayed_);
        }
        resize(scene()->sceneRect().width() + 4, scene()->sceneRect().height() + 4);
        show();
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void MessagePreviewWidget::displayImage(const QImage& img)
{
    if (img.isNull()) {
        hide();
        return;
    }

    if (txt_item_) {
        scene()->removeItem(txt_item_);
        delete txt_item_;
        txt_item_ = nullptr;
    }

    if (pm_item_) {
        pm_item_->setPixmap(QPixmap::fromImage(img));
    } else {
        pm_item_ = scene()->addPixmap(QPixmap::fromImage(img));
    }
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);

    show();
}

bool MessagePreviewWidget::isConnected() const
{
    return static_cast<bool>(connection_);
}

/// MOC
#include "../../../include/csapex/view/widgets/moc_message_preview_widget.cpp"
