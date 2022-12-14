#pragma once

#include <map>
#include <algorithm>
#include <math.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <mutex>

#include <QDebug>

class FaceDatabase
{

private:
    static std::mutex   mutexResource;
    std::map<std::string, std::map<std::string, std::vector<float>>> dbMapList;

    FaceDatabase(const FaceDatabase &) = delete;            //Prevent Copy
    FaceDatabase &operator=(const FaceDatabase &) = delete; //Prevent assignment
    ~FaceDatabase();
    FaceDatabase();

    float cosine_similarity(std::vector<float> a, std::vector<float> b);

public:

    static FaceDatabase &GetInstance();

    void AddFace(std::string GroupName, std::string key, std::vector<float> data);

    std::string FindBestMatch(std::string GroupName, std::vector<float> data, float scoreThreshold);

};

