#include "json.hpp"
#include "Connector.cpp"

using json = nlohmann::json;
using namespace std;
using Response = Connector::Response;

class Everest {
public:
    class Job {
    private:
        Everest *everest;
        json originalJSON;
    public:
        /*enum States {
        SUBMITTED, DEFERRED, READY, SCHEDULED, RUNNING, DONE, FAILED, CANCELLED
    };*/
        string id;
        string name;
        string app_id;
        //string resources[];
        string state;
        json inputs;
        json result;

        Job(Everest *everest) {
            this->everest = everest;
        }

        Job(Everest *everest, json j) {
            this->everest = everest;
            originalJSON = j;
            fromJson(j);
        }


        Job *fromJson(json jobJSON) {
            originalJSON = jobJSON;
            if (jobJSON.find("result") != jobJSON.end()) {
                result = jobJSON["result"];
            }
            id = jobJSON["id"];
            app_id = jobJSON["app"];
            name = jobJSON["name"];
            inputs = jobJSON["inputs"];
            state = jobJSON["state"];
            return this;
        }

        void refreshJob() {
            fromJson(everest->getJob(id)->originalJSON);
        }

        void cancelJob() {
            everest->cancelJob(id);
        }

        void deleteJob() {
            everest->deleteJob(id);
        }
    };

    class File {
    private:
        Everest *everest;
        json originalJSON;
    public:
        string filename;
        long length;
        string uri;

        File(Everest *everest) {
            this->everest = everest;
        }

        File(Everest *everest, json j) {
            this->everest = everest;
            originalJSON = j;
            fromJson(j);
        }

        File *fromJson(json fileJSON) {
            originalJSON = fileJSON;
            filename = fileJSON["filename"];
            length = fileJSON["length"];
            uri = fileJSON["uri"];
            return this;
        }

        void deleteFile() {
            everest->deleteFile(uri);
        }
    };

private:
    static constexpr const char *baseURL = "https://everest.distcomp.org";
    Connector *connector;
public:
    Everest() {
        connector = new Connector();
    }

    ~Everest() {
        delete connector;
    }

    //Token
    string getAccessToken(string username, string password, string label) {
        json j;
        j["username"] = username;
        j["password"] = password;
        j["label"] = label;
        string link = baseURL;
        link += "/auth/access_token";
        Response response = connector->postRequest(link, j.dump());
        if (response.code == 200) {
            json responseJSON = json::parse(response.response);
            return responseJSON["access_token"];
        } else throw runtime_error(string("getAccessToken crush. Error code: ") + to_string(response.code));
    }

    void removeAccessToken() {
        string link = baseURL;
        link += "/auth/access_token";
        Response response = connector->deleteRequest(link);
        if (response.code != 200)
            throw runtime_error(string("removeAccessToken crush. Error code: ") + to_string(response.code));
    }

    //Apps
    /*json getApps() {
        string link = baseURL;
        link += "/api/apps";
        Response response = connector->getRequest(link);
        if (response.code == 200) return json::parse(response.response);
        else throw runtime_error(string("getApps crush. Error code: ") + to_string(response.code));
    }*/

    json getApp(string id) {
        string link = baseURL;
        link += "/api/app/" + id;
        Response response = connector->getRequest(link);
        if (response.code == 200) return json::parse(response.response);
        else throw runtime_error(string("getApp crush. Error code: ") + to_string(response.code));
    }

    //Jobs
    vector<Job *> getJobs() {
        string link = baseURL;
        link += "/api/lobs";
        Response response = connector->getRequest(link);
        if (response.code == 200) {
            json arrOfJobs = json::parse(response.response);
            vector<Job *> jobs;
            for (auto &arrOfJob : arrOfJobs)
                jobs.push_back(new Job(this, arrOfJob));
            return jobs;
        } else throw runtime_error(string("getJobs crush. Error code: ") + to_string(response.code));
    }

    Job *getJob(string id) {
        string link = baseURL;
        link += "/api/job/" + id;
        Response response = connector->getRequest(link);
        if (response.code == 200) return new Job(this, json::parse(response.response));
        else throw runtime_error(string("getJob crush. Error code: ") + to_string(response.code));
    }

    void runJob(string app_id, string name, json inputs) {
        string link = baseURL;
        link += "/api/apps/" + app_id;
        json job;
        job["name"] = name;
        job["inputs"] = inputs;
        //job["resources"] = (resources);
        Response response = connector->postRequest(link, job.dump());
        if (response.code != 200)
            throw runtime_error(string("runJob crush. Error code: ") + to_string(response.code));
    }

    void cancelJob(string id) {
        string link = baseURL;
        link += "/api/job/" + id + "/cancel";
        Response response = connector->postRequest(link, "");
        if (response.code != 200)
            throw runtime_error(string("cancelJob crush. Error code: ") + to_string(response.code));
    }

    void deleteJob(string id) {
        string link = baseURL;
        link += "/api/job/" + id;
        Response response = connector->deleteRequest(link);
        if (response.code != 200)
            throw runtime_error(string("deleteJob crush. Error code: ") + to_string(response.code));
    }

    //Files
    File *uploadFile(string filepath) {
        string link = baseURL;
        link += "/api/files/temp";
        Response response = connector->uploadFile(link, filepath);
        if (response.code != 200)
            throw runtime_error(string("uploadFile crush. Error code: ") + to_string(response.code));
        return new File(this, json::parse(response.response));
    }

    void deleteFile(string uri) {
        string link = baseURL;
        link += uri;
        Response response = connector->deleteRequest(link);
        if (response.code != 200)
            throw runtime_error(string("deleteFile crush. Error code: ") + to_string(response.code));
    }
};