#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <memory>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;

    Submission(const string& p, const string& s, int t)
        : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    bool solved;
    int solved_time;
    int wrong_attempts;
    int frozen_submissions;
    bool is_frozen;

    ProblemStatus() : solved(false), solved_time(0), wrong_attempts(0),
                      frozen_submissions(0), is_frozen(false) {}
};

struct TeamStatus {
    string name;
    int solved_count;
    int total_penalty;
    vector<int> solved_times;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int last_flush_ranking;

    TeamStatus(const string& n) : name(n), solved_count(0), total_penalty(0),
                                  last_flush_ranking(0) {}

    void update_penalty() {
        solved_count = 0;
        total_penalty = 0;
        solved_times.clear();

        for (auto& [problem, status] : problems) {
            if (status.solved) {
                solved_count++;
                total_penalty += status.wrong_attempts * 20 + status.solved_time;
                solved_times.push_back(status.solved_time);
            }
        }

        sort(solved_times.rbegin(), solved_times.rend());
    }
};

struct RankingComparator {
    bool operator()(const TeamStatus* a, const TeamStatus* b) const {
        if (a->solved_count != b->solved_count) {
            return a->solved_count > b->solved_count;
        }

        if (a->total_penalty != b->total_penalty) {
            return a->total_penalty < b->total_penalty;
        }

        for (size_t i = 0; i < a->solved_times.size() && i < b->solved_times.size(); i++) {
            if (a->solved_times[i] != b->solved_times[i]) {
                return a->solved_times[i] < b->solved_times[i];
            }
        }

        return a->name < b->name;
    }
};

class ICPCManager {
private:
    map<string, unique_ptr<TeamStatus>> teams;
    vector<TeamStatus*> team_order;
    bool competition_started;
    bool competition_ended;
    bool scoreboard_frozen;
    int duration_time;
    int problem_count;
    int current_time;
    int freeze_time;

    vector<string> get_problem_list() const {
        vector<string> problems;
        for (int i = 0; i < problem_count; i++) {
            problems.push_back(string(1, 'A' + i));
        }
        return problems;
    }

    void flush_scoreboard() {
        for (auto& team : teams) {
            team.second->update_penalty();
        }

        team_order.clear();
        for (auto& [name, team] : teams) {
            team_order.push_back(team.get());
        }

        sort(team_order.begin(), team_order.end(), RankingComparator());

        for (size_t i = 0; i < team_order.size(); i++) {
            team_order[i]->last_flush_ranking = i + 1;
        }
    }

    string get_problem_display(const TeamStatus* team, const string& problem) const {
        const ProblemStatus& status = team->problems.at(problem);

        if (status.is_frozen) {
            if (status.wrong_attempts == 0) {
                return "0/" + to_string(status.frozen_submissions);
            } else {
                return "-" + to_string(status.wrong_attempts) + "/" +
                       to_string(status.frozen_submissions);
            }
        } else if (status.solved) {
            if (status.wrong_attempts == 0) {
                return "+";
            } else {
                return "+" + to_string(status.wrong_attempts);
            }
        } else {
            if (status.wrong_attempts == 0) {
                return ".";
            } else {
                return "-" + to_string(status.wrong_attempts);
            }
        }
    }

    void print_scoreboard() {
        vector<string> problems = get_problem_list();

        for (TeamStatus* team : team_order) {
            cout << team->name << " " << team->last_flush_ranking << " "
                 << team->solved_count << " " << team->total_penalty;

            for (const string& prob : problems) {
                cout << " " << get_problem_display(team, prob);
            }
            cout << "\n";
        }
    }

    TeamStatus* find_team_with_lowest_rank_and_frozen() {
        for (int i = team_order.size() - 1; i >= 0; i--) {
            TeamStatus* team = team_order[i];
            for (auto& [prob, status] : team->problems) {
                if (status.is_frozen) {
                    return team;
                }
            }
        }
        return nullptr;
    }

    string find_smallest_frozen_problem(TeamStatus* team) {
        string smallest_prob;
        bool found = false;

        for (auto& [prob, status] : team->problems) {
            if (status.is_frozen) {
                if (!found || prob < smallest_prob) {
                    smallest_prob = prob;
                    found = true;
                }
            }
        }

        return smallest_prob;
    }

public:
    ICPCManager() : competition_started(false), competition_ended(false),
                    scoreboard_frozen(false), duration_time(0), problem_count(0),
                    current_time(0), freeze_time(0) {}

    void add_team(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }

        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        teams[team_name] = make_unique<TeamStatus>(team_name);
        cout << "[Info]Add successfully.\n";
    }

    void start_competition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }

        duration_time = duration;
        problem_count = problems;
        competition_started = true;

        vector<string> problem_list = get_problem_list();
        for (auto& [name, team] : teams) {
            for (const string& prob : problem_list) {
                team->problems[prob] = ProblemStatus();
            }
        }

        flush_scoreboard();
        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& team_name,
                const string& status, int time) {
        current_time = time;

        TeamStatus* team = teams[team_name].get();
        ProblemStatus& prob_status = team->problems[problem];

        team->submissions.emplace_back(problem, status, time);

        if (scoreboard_frozen && !prob_status.solved) {
            prob_status.frozen_submissions++;
            if (!prob_status.is_frozen) {
                prob_status.is_frozen = true;
            }
        } else if (!prob_status.solved) {
            if (status == "Accepted") {
                prob_status.solved = true;
                prob_status.solved_time = time;
            } else {
                prob_status.wrong_attempts++;
            }
        }
    }

    void flush() {
        flush_scoreboard();
        cout << "[Info]Flush scoreboard.\n";
    }

    void freeze() {
        if (scoreboard_frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }

        scoreboard_frozen = true;
        freeze_time = current_time;
        flush_scoreboard();
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!scoreboard_frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        flush_scoreboard();
        print_scoreboard();

        while (true) {
            TeamStatus* team = find_team_with_lowest_rank_and_frozen();
            if (!team) break;

            string problem = find_smallest_frozen_problem(team);

            // Unfreeze this problem
            ProblemStatus& status = team->problems[problem];
            status.is_frozen = false;

            // Process submissions after freeze time
            vector<Submission> relevant_submissions;
            for (const auto& sub : team->submissions) {
                if (sub.problem == problem && sub.time > freeze_time) {
                    relevant_submissions.push_back(sub);
                }
            }

            for (const auto& sub : relevant_submissions) {
                if (!status.solved) {
                    if (sub.status == "Accepted") {
                        status.solved = true;
                        status.solved_time = sub.time;
                    } else {
                        status.wrong_attempts++;
                    }
                }
            }

            // Store the current state before flushing
            map<string, int> old_rankings;
            for (auto* t : team_order) {
                old_rankings[t->name] = t->last_flush_ranking;
            }

            flush_scoreboard();
            int new_ranking = team->last_flush_ranking;

            if (old_rankings[team->name] != new_ranking) {
                // Find which team's position this team took
                string displaced_team_name;
                for (auto& [name, rank] : old_rankings) {
                    if (rank == new_ranking && name != team->name) {
                        displaced_team_name = name;
                        break;
                    }
                }

                if (!displaced_team_name.empty()) {
                    cout << team->name << " " << displaced_team_name << " "
                         << team->solved_count << " " << team->total_penalty << "\n";
                }
            }
        }

        scoreboard_frozen = false;
        print_scoreboard();
    }

    void query_ranking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";

        if (scoreboard_frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        TeamStatus* team = teams[team_name].get();
        cout << team_name << " NOW AT RANKING " << team->last_flush_ranking << "\n";
    }

    void query_submission(const string& team_name, const string& problem,
                         const string& status) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        TeamStatus* team = teams[team_name].get();
        Submission* last_match = nullptr;

        for (auto& sub : team->submissions) {
            bool problem_match = (problem == "ALL" || sub.problem == problem);
            bool status_match = (status == "ALL" || sub.status == status);

            if (problem_match && status_match) {
                last_match = &sub;
            }
        }

        if (!last_match) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << team_name << " " << last_match->problem << " "
                 << last_match->status << " " << last_match->time << "\n";
        }
    }

    void end() {
        cout << "[Info]Competition ends.\n";
        competition_ended = true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCManager manager;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            manager.add_team(team_name);
        } else if (command == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            manager.start_competition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, dummy1, team, dummy2, status, dummy3;
            int time;
            iss >> problem >> dummy1 >> team >> dummy2 >> status >> dummy3 >> time;
            manager.submit(problem, team, status, time);
        } else if (command == "FLUSH") {
            manager.flush();
        } else if (command == "FREEZE") {
            manager.freeze();
        } else if (command == "SCROLL") {
            manager.scroll();
        } else if (command == "QUERY_RANKING") {
            string team;
            iss >> team;
            manager.query_ranking(team);
        } else if (command == "QUERY_SUBMISSION") {
            string team, dummy1, condition;
            iss >> team >> dummy1 >> condition;

            string problem = "ALL";
            string status = "ALL";

            size_t prob_pos = condition.find("PROBLEM=");
            size_t status_pos = condition.find("STATUS=");

            if (prob_pos != string::npos) {
                size_t start = prob_pos + 8;
                size_t end = condition.find(" AND", start);
                if (end == string::npos) end = condition.length();
                problem = condition.substr(start, end - start);
            }

            if (status_pos != string::npos) {
                size_t start = status_pos + 7;
                if (prob_pos != string::npos) {
                    size_t and_pos = condition.find(" AND");
                    if (and_pos != string::npos && and_pos < status_pos) {
                        // PROBLEM=... AND STATUS=...
                        size_t status_start = status_pos + 7;
                        status = condition.substr(status_start);
                    } else if (and_pos != string::npos && and_pos > status_pos) {
                        // STATUS=... AND PROBLEM=...
                        size_t status_end = condition.find(" AND", start);
                        if (status_end == string::npos) {
                            status = condition.substr(start);
                        } else {
                            status = condition.substr(start, status_end - start);
                        }
                    } else {
                        status = condition.substr(start);
                    }
                } else {
                    status = condition.substr(start);
                }
            }

            manager.query_submission(team, problem, status);
        } else if (command == "END") {
            manager.end();
            break;
        }
    }

    return 0;
}