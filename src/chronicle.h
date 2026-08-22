#pragma once
#include <string>
#include <vector>

struct NewsArticle {
    float       sim_time = 0.0f;
    std::string headline;
    std::string body;
    std::string category = "WAR"; // WAR, ALLIANCE, DECREE, HEROIC
};

class Chronicle {
public:
    Chronicle() = default;

    void add_article(float sim_time, const std::string& headline, const std::string& body, const std::string& category = "WAR") {
        NewsArticle a;
        a.sim_time = sim_time;
        a.headline = headline;
        a.body = body;
        a.category = category;
        if (articles_.size() >= 100) articles_.erase(articles_.begin());
        articles_.push_back(a);
    }

    const std::vector<NewsArticle>& articles() const { return articles_; }

private:
    std::vector<NewsArticle> articles_;
};
