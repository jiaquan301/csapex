#ifndef CV_DECOMPOSITION_CLASSIFIER_HPP
#define CV_DECOMPOSITION_CLASSIFIER_HPP
#include <opencv2/core/core.hpp>
/**
 * @brief The DecompositionClassifier class is used for classification of rectangular areas
 *        within an image, determining, wether with what algorithm, if an area should be splitted
 *        or not.
 */
/// interface
class DecompositionClassifier
{
public:
    /**
     * @brief Classify a region within in an image.
     * @param roi       an region of interest
     * @return      if decomposition is required
     */
    virtual bool classify(const cv::Rect &roi) = 0;
    /**
     * @brief Set the image reference to classify.
     * @param image     an image
     */
    virtual void set_image(const cv::Mat &image) = 0;
protected:
    DecompositionClassifier()
    {
    }
};

/**
 * @brief This class can be used to classify a rectangular area. In this case the classification is
 *        defined by the maximum difference of the grey values.
 */
class GreyValueClassifier : public DecompositionClassifier
{
public:
    GreyValueClassifier(const int _threshold) :
        threshold_(_threshold)
    {
    }

    virtual ~GreyValueClassifier()
    {
    }
    bool classify(const cv::Rect &roi)
    {
        double min, max;
        cv::Mat image(grey_image_, roi);
        cv::minMaxLoc(image, &min, &max);

        return (max - min) > threshold_;
    }

    void set_image(const cv::Mat &image)
    {
        cv::cvtColor(image, grey_image_, CV_BGR2GRAY);
    }

private:
    int     threshold_;
    cv::Mat grey_image_;
};

#endif // DECOMPOSITION_CLASSIFIER_HPP