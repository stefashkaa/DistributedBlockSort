#include "Connector.cpp"
#include "json.hpp"

using json = nlohmann::json;
using Response = Connector::Response;

class Everest {

public:
    enum State {
        SUBMITTED,
        DEFERRED,
        READY,
        SCHEDULED,
        RUNNING,
        DONE,
        FAILED,
        CANCELLED,
        UNKNOWN
    };

    class Job {

    private:
        Everest *everest;
        json originalJSON;

        State getState(string state) {
            if(state == "SUBMITTED") return State::SUBMITTED;
            else if(state == "READY") return State::READY;
            else if(state == "SCHEDULED") return State::SCHEDULED;
            else if(state == "RUNNING") return State::RUNNING;
            else if(state == "DONE") return State::DONE;
            else if(state == "FAILED") return State::FAILED;
            else if(state == "CANCELLED") return State::CANCELLED;
            else if(state == "DEFERRED") return State::DEFERRED;
            return State::UNKNOWN;
        }

    public:
        string id;
        string name;
        string app_id;
        State state;
        json inputs;
        json result;
        int references;

        Job(Everest *everest) {
            this->everest = everest;
        }

        Job(Everest *everest, json j) {
            this->everest = everest;
            fromJson(j);
        }

        Job *fromJson(json jobJSON) {
            originalJSON = jobJSON;

            if (jobJSON.find("result") != jobJSON.end()) {
                result = jobJSON["result"];
            }

            id = jobJSON["id"];
            app_id = jobJSON["appId"];
            name = jobJSON["appName"];
            inputs = jobJSON["inputs"];
            state = getState(jobJSON["state"]);

            return this;
        }

        void refresh() {
            fromJson(everest->getJob(id)->originalJSON);
        }

        void cancel() {
            everest->cancelJob(id);
        }

        void remove() {
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

        void remove() {
            everest->deleteFile(uri);
        }
    };

private:

    static constexpr const char *baseURL = "https://everest.distcomp.org";
    Connector *connector;

public:
    Everest(string username, string password, string label) {
        connector = new Connector();
        cout << "Token: " << getAccessToken(username, password, label) << endl;
    }

    ~Everest() {
        delete connector;
    }

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
        } else {
            throw runtime_error(string("getAccessToken crush. Error code: ") +
                to_string(response.code));
        }
    }

    void removeAccessToken() {
        connector = new Connector();

        string link = baseURL;
        link += "/auth/access_token";

        Response response = connector->deleteRequest(link);

        if (response.code != 200) {
            throw runtime_error(string("removeAccessToken crush. Error code: ") +
                to_string(response.code));
        }
    }

    json getApps() {
        connector = new Connector();

        string link = baseURL;
        link += "/api/apps/search?pagenum=1&pagesize=20&sortBy=jobs";

        Response response = connector->getRequest(link);

        if (response.code == 200) {
            return json::parse(response.response);
        } else {
            throw runtime_error(string("getApps crush. Error code: ") +
                to_string(response.code));
        }
    }

    json getApp(string id) {
        connector = new Connector();

        string link = baseURL;
        link += "/api/apps/" + id;

        Response response = connector->getRequest(link);

        if (response.code == 200) {
            return json::parse(response.response);
        } else {
            throw runtime_error(string("getApp crush. Error code: ") +
                to_string(response.code));
        }
    }

    vector<Job*> getJobs() {
        connector = new Connector();

        string link = baseURL;
        link += "/api/jobs";

        Response response = connector->getRequest(link);

        if (response.code == 200) {
            json arrOfJobs = json::parse(response.response);
            vector<Job*> jobs;

            for (auto &arrOfJob : arrOfJobs){
                jobs.push_back(new Job(this, arrOfJob));
            }
            return jobs;
        } else {
            throw runtime_error(string("getJobs crush. Error code: ") +
                to_string(response.code));
        }
    }

    Job* getJob(string id) {
        connector = new Connector();

        string link = baseURL;
        link += "/api/jobs/" + id;

        Response response = connector->getRequest(link);

        if (response.code == 200) {
            return new Job(this, json::parse(response.response));
        } else {
            throw runtime_error(string("getJob crush. Error code: ") +
                to_string(response.code));
        }
    }

    Job* runJob(string app_id, string name, json inputs) {
        connector = new Connector();

        string link = baseURL;
        link += "/api/apps/" + app_id;
        json job;
        job["name"] = name;
        job["inputs"] = inputs;

        Response response = connector->postRequest(link, job.dump());

        if (response.code != 200 && response.code != 201) {
            throw runtime_error(string("runJob crush. Error code: ") +
                to_string(response.code) + "\n" + response.response);
        } else {
            return new Job(this, json::parse(response.response));
        }
    }

    void cancelJob(string id) {
        connector = new Connector();

        string link = baseURL;
        link += "/api/jobs/" + id + "/cancel";

        Response response = connector->postRequest(link, "");

        if (response.code != 200) {
            throw runtime_error(string("cancelJob crush. Error code: ") +
                to_string(response.code));
        }
    }

    void deleteJob(string id) {
        connector = new Connector();

        string link = baseURL;
        link += "/api/jobs/" + id;

        Response response = connector->deleteRequest(link);

        if (response.code != 200) {
            throw runtime_error(string("deleteJob crush. Error code: ") +
                to_string(response.code));
        }
    }

    File* uploadFile(string fullName) {
        connector = new Connector();

        string link = baseURL;
        link += "/api/files/temp";

        Response response = connector->uploadFile(link, fullName);

        if (response.code != 200) {
            throw runtime_error(string("uploadFile crush. Error code: ") +
                to_string(response.code));
        }

        return new File(this, json::parse(response.response));
    }

    string downloadFile(string fullName) {
        connector = new Connector();

        string link = baseURL;
        link += fullName;
        Response response = connector->getRequest(link);
        if (response.code != 200) {
            throw runtime_error(string("uploadFile crush. Error code: ") +
                to_string(response.code));
        }
        return response.response;
    }

    void deleteFile(string uri) {
        connector = new Connector();

        string link = baseURL;
        link += uri;
        Response response = connector->deleteRequest(link);
        if (response.code != 200) {
            throw runtime_error(string("deleteFile crush. Error code: ") +
                to_string(response.code));
        }
    }

    void deleteAllFiles() {
        connector = new Connector();

        string link = baseURL;
        link += "/api/user/tmp_files";
        Response response = connector->deleteRequest(link);
        if (response.code != 200) {
            throw runtime_error(string("deleteFile crush. Error code: ") +
                to_string(response.code));
        }
    }
};