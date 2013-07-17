#include "combiner_gridcompare_hist.h"

/// SYSTEM
#include <pluginlib/class_list_macros.h>
#include <QComboBox>

PLUGINLIB_EXPORT_CLASS(vision_evaluator::GridCompareHist, vision_evaluator::ImageCombiner)

using namespace vision_evaluator;
using namespace cv_grid;

GridCompareHist::GridCompareHist() :
    GridCompare(State::Ptr(new State)),
    container_hist_sliders_(NULL)
{
    private_state_ = dynamic_cast<State*>(state_.get());
    assert(private_state_);

}


cv::Mat GridCompareHist::combine(const cv::Mat img1, const cv::Mat mask1, const cv::Mat img2, const cv::Mat mask2)
{
    if(!img1.empty() && !img2.empty()) {
        if(img1.channels() != img2.channels())
            throw std::runtime_error("Channel count is not matching!");

        if(private_state_->channel_count != img1.channels()) {
            private_state_->channel_count = img1.channels();
            private_state_->bins.clear();
            private_state_->eps.clear();
            Q_EMIT modelChanged();
        }

        if(hist_sliders_.size() == private_state_->channel_count) {
            GridHist g1, g2;
            AttrHistogram::Params p;
            prepareHistParams(p.bins, p.ranges, p.eps);
            int index = combo_compare_->currentIndex();
            p.method = index_to_compare_[index];

            int width = slide_width_->value();
            int height = slide_height_->value();

            cv_grid::prepare_grid<AttrHistogram>(g1, img1, height, width, p, mask1, 1.0);
            cv_grid::prepare_grid<AttrHistogram>(g2, img2, height, width, p, mask2, 1.0);

            cv::Mat out(img1.rows + 40, img1.cols, CV_8UC3, cv::Scalar(0,0,0));
            render_grid(g1, g2, out);
            return out;
        }
    }
    return cv::Mat();
}

void GridCompareHist::updateGui(QBoxLayout *layout)
{
    QVBoxLayout *internal_layout;
    if(container_hist_sliders_ != NULL) {
        container_hist_sliders_->deleteLater();
    }
    internal_layout = new QVBoxLayout;

    for(int i = 0 ; i < private_state_->channel_count ; i++) {
        std::stringstream ch;
        ch << i + 1;

        int    default_bin = 32;
        double default_eps = 0.0;

        if(private_state_->restored) {
            default_bin = private_state_->bins[i];
            default_eps = private_state_->eps[i];
        } else {
            private_state_->bins.push_back(default_bin);
            private_state_->eps.push_back(default_eps);
        }

        QSlider *bins = QtHelper::makeSlider(internal_layout, "Ch." + ch.str() + " bins", default_bin, 1, 1000);
        QDoubleSlider *eps = QtHelper::makeDoubleSlider(internal_layout, "Ch." + ch.str() + " eps", default_eps, 0.0, 255.0, 0.01);
        insertSliders(bins, eps);
    }

    container_hist_sliders_ = QtHelper::wrapLayout(internal_layout);
    layout->addWidget(container_hist_sliders_);


}

Memento::Ptr GridCompareHist::getState() const
{
    State::Ptr memento(new State);
    *memento = *state_;

    return memento;
}

void GridCompareHist::setState(Memento::Ptr memento)
{
    state_.reset(new State);
    State::Ptr s = boost::dynamic_pointer_cast<State>(memento);
    assert(s.get());
    *state_ = *s;
    assert(state_.get());
     private_state_ = dynamic_cast<State*>(state_.get());
    assert(private_state_);

    slide_height_->setValue(private_state_->grid_height);
    slide_width_->setValue(private_state_->grid_width);
    combo_compare_->setCurrentIndex(private_state_->combo_index);

    private_state_->restored = true;

    Q_EMIT modelChanged();
}

void GridCompareHist::updateState(int value)
{
    private_state_->combo_index = value;
}

void GridCompareHist::updateState()
{
    private_state_->grid_width  = slide_width_->value();
    private_state_->grid_height = slide_height_->value();
}

void GridCompareHist::fill(QBoxLayout *layout)
{
    GridCompare::fill(layout);
    private_state_->restored = false;

    combo_compare_ = new QComboBox();
    combo_compare_->addItem("Correlation");
    combo_compare_->addItem("Chi-Square");
    combo_compare_->addItem("Intersection");
    combo_compare_->addItem("Hellinger");
    combo_compare_->addItem("Squared Distances");

    int index = combo_compare_->findText("Correlation");
    index_to_compare_.insert(intPair(index, CV_COMP_CORREL));
    index = combo_compare_->findText("Chi-Square");
    index_to_compare_.insert(intPair(index, CV_COMP_CHISQR));
    index = combo_compare_->findText("Intersection");
    index_to_compare_.insert(intPair(index, CV_COMP_INTERSECT));
    index = combo_compare_->findText("Hellinger");
    index_to_compare_.insert(intPair(index, CV_COMP_BHATTACHARYYA));
    index = combo_compare_->findText("Squared Distances");
    index_to_compare_.insert(intPair(index, AttrHistogram::CV_COMP_SQRD));

    layout->addWidget(combo_compare_);
    private_state_->combo_index = combo_compare_->currentIndex();

    connect(combo_compare_, SIGNAL(currentIndexChanged(int)), this, SLOT(updateState(int)));
    connect(slide_height_, SIGNAL(sliderReleased()), this, SLOT(updateState()));
    connect(slide_width_, SIGNAL(sliderReleased()), this, SLOT(updateState()));

}

void GridCompareHist::insertSliders(QSlider *bins, QDoubleSlider *eps)
{
    HistSliderPair p;
    p.first = bins;
    p.second = eps;
    hist_sliders_.push_back(p);
}

void GridCompareHist::prepareHistParams(cv::Mat &bins, cv::Mat &ranges, cv::Scalar &eps)
{
    bins = cv::Mat_<int>(private_state_->channel_count, 1);
    ranges = cv::Mat_<float>(private_state_->channel_count * 2 ,1);
    for(int i = 0 ; i < private_state_->channel_count ; i++) {
        HistSliderPair p = hist_sliders_[i];
        bins.at<int>(i)     = p.first->value();
        ranges.at<float>(2 * i)     = 0.f;
        ranges.at<float>(2 * i + 1) = 256.f;
        eps[i]          = p.second->doubleValue();

        /// MEMENTO
        private_state_->bins[i] = p.first->value();
        private_state_->eps[i] = p.second->doubleValue();
    }
}

/// MEMENTO
void GridCompareHist::State::readYaml(const YAML::Node &node)
{
    GridCompare::State::readYaml(node);
    node["compare"] >> combo_index;

    const YAML::Node &_bins = node["bins"];
    for(YAML::Iterator it = _bins.begin() ; it != _bins.end() ; it++) {
        int bin_val;
        *it >> bin_val;
        bins.push_back(bin_val);
    }

    const YAML::Node &_eps = node["eps"];
    for(YAML::Iterator it = _eps.begin() ; it != _eps.end() ; it++) {
        double eps_val;
        *it >> eps_val;
        eps.push_back(eps_val);
    }
}

void GridCompareHist::State::writeYaml(YAML::Emitter &out) const
{
    GridCompare::State::writeYaml(out);

    out << YAML::Key << "compare" << YAML::Value << combo_index;
    out << YAML::Key << "bins" << YAML::Value << YAML::BeginSeq;
    for(std::vector<int>::const_iterator it = bins.begin() ; it != bins.end() ; it++) {
        out << *it;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "eps" << YAML::Value << YAML::BeginSeq;
    for(std::vector<double>::const_iterator it = eps.begin() ; it != eps.end() ; it++) {
        out << *it;
    }
    out << YAML::EndSeq;

}
