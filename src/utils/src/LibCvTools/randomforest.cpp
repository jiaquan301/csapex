#include "randomforest.h"

RandomForest::RandomForest() :
    is_trained_(false)
{
}

bool RandomForest::load(const std::string &path)
{
    try {
        forest_->load(path.c_str());
        is_trained_ = true;
        return true;
    } catch (cv::Exception e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool RandomForest::isTrained()
{
    return is_trained_;
}

void RandomForest::predictClass(const cv::Mat &sample, int &classID)
{
    classID = forest_->predict(sample);
}

void RandomForest::predictClassProb(const cv::Mat &sample, int &classID, float &prob)
{
    std::map<int, float> probs;
    prediction(sample, probs, classID);
    prob    = probs[classID];
}

void RandomForest::predictClassProbs(const cv::Mat &sample, std::vector<int> &classIDs, std::vector<float> &probs)
{
    std::map<int, float> probs_map;
    int classID;
    prediction(sample, probs_map, classID);
    for(std::map<int, float>::iterator it = probs_map.begin() ; it != probs_map.end() ; it++) {
        classIDs.push_back(it->first);
        probs.push_back(it->second);
    }
}

void RandomForest::prediction(const cv::Mat &sample, std::map<int, float> &probs, int &maxClassID)
{
    int ntrees = forest_->get_tree_count();
    std::map<int,float> votes;

    /// COUNT
    int max_vote = 0;
    for(int i = 0 ; i < ntrees ; i++) {
        CvDTreeNode* prediction = forest_->get_tree(i)->predict(sample);
        int tree_classID = prediction->value;

        if(votes.find(tree_classID) == votes.end()) {
            votes.insert(std::pair<int,float>(tree_classID, 0));
        }

        votes[tree_classID] += 1;
        if(votes[tree_classID] > max_vote) {
            max_vote    = votes[tree_classID];
            maxClassID  = tree_classID;
        }
    }

    /// NORMALIZE
    for(std::map<int,float>::iterator it = votes.begin() ; it != votes.end() ; it++) {
        it->second /= (float) ntrees;
    }

    probs = votes;
}

