#include "FaceDatabase.hpp"


std::mutex FaceDatabase::mutexResource;

FaceDatabase::~FaceDatabase()
{

};


FaceDatabase::FaceDatabase()
{

};

FaceDatabase &FaceDatabase::GetInstance() {
    std::lock_guard<std::mutex> lock(mutexResource);
    static FaceDatabase instance;
    return instance;
};

void FaceDatabase::AddFace(std::string GroupName, std::string key, std::vector<float> data)
{

    std::lock_guard<std::mutex> lock(mutexResource);

    if (dbMapList.find(GroupName) == dbMapList.end()) {
        //Not found
        dbMapList[GroupName][key] = data;
    }
    else {
        //Found
        std::map<std::string, std::vector<float>> &dbMapGroup = dbMapList.at(GroupName);
        dbMapGroup[key] = data;
    }
};


std::string FaceDatabase::FindBestMatch(std::string GroupName, std::vector<float> data, float scoreThreshold)
{

    std::string BestMatchKey;
    float       best_score = 0.0f;
    std::lock_guard<std::mutex> lock(mutexResource);

    if (dbMapList.find(GroupName) != dbMapList.end()) {
        std::map<std::string, std::vector<float>> dbMapGroup = dbMapList.at(GroupName);
        for(const auto& item: dbMapGroup)
        {
            float score = cosine_similarity(data, item.second);

            if ((score >= scoreThreshold) && (score > best_score)) {
                best_score = score;
                BestMatchKey = item.first;
                //qDebug() << "Face Best = " << BestMatchKey.c_str() << ", score = " << best_score;
            }
        }
    }

    return BestMatchKey;
};


//http://mlwiki.org/index.php/Cosine_Similarity
//https://www.machinelearningplus.com/nlp/cosine-similarity/
float FaceDatabase::cosine_similarity(std::vector<float> a, std::vector<float> b)
{
    size_t len = a.size();
    float dot = 0;
    float denom_a = 0;
    float denom_b = 0;
    for (size_t i = 0; i < len; i++)
    {
        dot += a[i] * b[i];
        denom_a += a[i] * a[i];
        denom_b += b[i] * b[i];
    }

    return dot / (sqrt(denom_a) * sqrt(denom_b));
};
