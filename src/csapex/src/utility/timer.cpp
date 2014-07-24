/// HEADER
#include <csapex/utility/timer.h>

using namespace csapex;

Timer::Timer(const std::string& name)
    : timer_name_(name), root(new Interval(name))
{
    active.push_back(root);
}
Timer::~Timer()
{
    finish();
}

std::vector<std::pair<std::string, int> > Timer::entries() const
{
    assert(active.empty());

    std::vector<std::pair<std::string, int> > result;
    root->entries(result);
    return result;
}

void Timer::finish()
{
    while(!active.empty()) {
        active.back()->stop();
        active.pop_back();
    }
}

Timer::Interlude::Ptr Timer::step(const std::string &name)
{
    return Timer::Interlude::Ptr(new Timer::Interlude(this, name));
}

Timer::Interlude::Interlude(Timer *parent, const std::string &name)
    : parent_(parent)
{
    // start new interval in timer
    Interval::Ptr i(new Interval(name));
    parent_->active.back()->sub.push_back(i);
    parent_->active.push_back(i);
}

Timer::Interlude::~Interlude()
{
    // stop interval
    Interval::Ptr i = parent_->active.back();
    i->stop();
    parent_->active.pop_back();
}

void Timer::Interval::entries(std::vector<std::pair<std::string, int> > &out) const
{
    out.push_back(std::make_pair(name_, lengthMs()));
    for(std::vector<Interval::Ptr>::const_iterator it = sub.begin(); it != sub.end(); ++it) {
        (*it)->entries(out);
    }
}

int Timer::Interval::lengthMs() const
{
    return end_.toMSecsSinceEpoch() - start_.toMSecsSinceEpoch();
}

int Timer::Interval::lengthSubMs() const
{
    int sum = 0;
    for(std::vector<Interval::Ptr>::const_iterator it = sub.begin(); it != sub.end(); ++it) {
        const Interval& i = **it;
        sum += i.lengthMs();
    }
    return sum;
}

Timer::Interval::Interval(const std::string &name)
    : name_(name)
{
    start();
}

std::string Timer::Interval::name() const
{
    return name_;
}

void Timer::Interval::start()
{
    start_ = QDateTime::currentDateTime();
}

void Timer::Interval::stop()
{
    end_ = QDateTime::currentDateTime();
}
