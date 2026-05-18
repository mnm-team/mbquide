#include <crow.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "MBQC_Graph.hpp"
#include "QASM_Parser.hpp"
#include "ZX2MBQC.hpp"
#include "MBQC2ZX.hpp"
#include "Circ2MBQC.hpp"
#include "Flow.hpp"
#include "OutputAdjustments.hpp"
#include "Simulator.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

// Helper to get MIME type
std::string get_mime_type(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".svg"))  return "image/svg+xml";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".ico"))  return "image/x-icon";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".woff2")) return "font/woff2";
    return "application/octet-stream";
}

// Helper to read a file into a string
std::optional<std::string> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Session map: stores objects by session ID
std::unordered_map<std::string, MBQC_Graph> user_graphs;
std::unordered_map<std::string, PauliFlowResult> user_flows;
std::unordered_map<std::string, Simulator> user_simulators;
std::unordered_map<std::string, ZXGraph> user_zx_graphs;


// Function to generate a unique session ID (simple random example)
std::string generate_session_id() {
    srand(time(0));
    char session_id[32];
    for (int i = 0; i < 32; ++i) {
        session_id[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"[rand() % 62];
    }
    return std::string(session_id, 32);
}

// Function to retrieve cookies from request headers
std::string get_cookie(const crow::request& req, const std::string& cookie_name) {
    auto cookies = req.get_header_value("Cookie");
    size_t pos = cookies.find(cookie_name + "=");
    if (pos != std::string::npos) {
        size_t start = pos + cookie_name.length() + 1;
        size_t end = cookies.find(";", start);
        if (end == std::string::npos) {
            end = cookies.length();
        }
        return cookies.substr(start, end - start);
    }
    return "";
}

struct SessionInfo {
    std::string id;
    bool is_new;
};

SessionInfo get_session_id(const crow::request& req) {
    std::string session_id = get_cookie(req, "session_id");
    if (session_id.empty()) {
        session_id = generate_session_id();
        return {session_id, true};
    }
    return {session_id, false};
}


// Handle session and graph initialization
MBQC_Graph& get_graph_for_session(const std::string& session_id) {
    if (user_graphs.find(session_id) == user_graphs.end()) {
        MBQC_Graph graph(4, {0}, {3});
        graph.addEdge(0,1);
        graph.addEdge(0,2);
        graph.addEdge(2,1);
        graph.addEdge(1,3);

        graph.setMeasurement(0, MeasurementBasis::Y, 0);
        graph.setMeasurement(1, MeasurementBasis::XY, 3*M_PI/4);
        graph.setMeasurement(2, MeasurementBasis::XZ, M_PI/4);
        
        user_graphs[session_id] = graph;
    }
    return user_graphs[session_id];
}

// Handle flow
PauliFlowResult& get_flow_for_session(const std::string& session_id) {
    if (user_flows.find(session_id) == user_flows.end()) {
        user_flows[session_id] = PauliFlowResult(); 
    }
    return user_flows[session_id];
}

// Handle simulator
Simulator& get_simulator_for_session(const std::string& session_id) {
    if (user_simulators.find(session_id) == user_simulators.end()) {
        user_simulators[session_id] = Simulator(); 
    }
    return user_simulators[session_id];
}


// Handle ZX Grpah session and initialization
ZXGraph& get_zx_for_session(const std::string& session_id) {

    if (user_zx_graphs.find(session_id) == user_zx_graphs.end()) {
        user_zx_graphs[session_id] = ZXGraph();
    }
    
    return user_zx_graphs[session_id];
};


struct CORS {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "http://localhost:5173");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        res.add_header("Access-Control-Allow-Credentials", "true");
    }

    void after_handle(crow::request& /*req*/, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "http://localhost:5173");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        res.add_header("Access-Control-Allow-Credentials", "true");
    }
};

int main() {
    crow::App<CORS> app;

    CROW_ROUTE(app, "/api/graph").methods(crow::HTTPMethod::Options)
    ([](const crow::request& /*req*/, crow::response& res) {
        res.add_header("Access-Control-Allow-Origin", "http://localhost:5173,*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        res.add_header("Access-Control-Allow-Credentials", "true");

        res.code = 204; // No Content
        res.end();
    });
    
    CROW_ROUTE(app, "/api/graph").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionInfo session = get_session_id(req);
        MBQC_Graph& graph = get_graph_for_session(session.id);
        json j = graph.toJson();
        
        crow::response res{j.dump()};
        res.set_header("Content-Type", "application/json");
        if (session.is_new) {
            res.add_header("Set-Cookie", "session_id=" + session.id + "; Path=/; HttpOnly; Max-Age=86400; SameSite=None; Secure");
        }
        return res;
    });

    CROW_ROUTE(app, "/api/graph").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {

        SessionInfo session = get_session_id(req);
        MBQC_Graph& graph = get_graph_for_session(session.id);

        json j = graph.toJson();

        
        try {
            auto body = json::parse(req.body);

            if (body.contains("size")) {
                graph = MBQC_Graph::fromJson(body);
                j = graph.toJson();

            } else if (body.contains("transform")) {
                ZXGraph& zx = get_zx_for_session(session.id);
                zx = MBQCtoZXGraph(get_graph_for_session(session.id));

                j = zx.toJson();

            } else if (body.contains("operation")) {
                std::string op = body["operation"];
                if (op == "lc") {
                    int u = body["node"];
                    graph.localComplementation(u);
                } else if (op == "pivot") {
                    int u = body["u"];
                    int v = body["v"];
                    graph.pivot(u, v);
                } else if (op == "z-insert") {
                    std::vector<int> ids = body["ids"];
                    graph.ZInsertion(ids);
                } else if (op == "z-delete") {
                    std::vector<int> ids = body["ids"];
                    graph.ZDeletion(ids);
                } else if (op == "relabel") {
                    int u = body["node"];
                    graph.relabel(u);
                } else if (op == "relabel-planar") {
                    int u = body["node"];
                    if (body.contains("pref-basis")) {
                        MeasurementBasis preferredBasis = parseBasis(body["pref-basis"]);
                        graph.relabelPlanar(u, preferredBasis);
                    } else {
                        graph.relabelPlanar(u);
                    }
                }

                j = graph.toJson();

            } else if (body.contains("simplify")) {
                graph.simplify();
                j = graph.toJson();

            } else if (body.contains("flow")) {
                std::string flow = body["flow"];
                PauliFlowResult& pauliFlow = get_flow_for_session(session.id);

                if (flow == "pauli") {
                    pauliFlow = findPauliFlow(graph);
                    j = graph.toJson();
                    j["flow"] = (json)PauliFlowResultToJson(pauliFlow);
                }
                if (flow == "focus") {
                    if (pauliFlow.ok) {
                        focus(pauliFlow, graph);
                        j = graph.toJson();
                        j["flow"] = (json)PauliFlowResultToJson(pauliFlow);
                    }
                }

            } else if (body.contains("simulate")) {
                MBQC_Graph& graph = get_graph_for_session(session.id);
                PauliFlowResult& flow = get_flow_for_session(session.id);
                Simulator& simulator = get_simulator_for_session(session.id);

                bool random = body["random"];
                std::string inputState = body["input"];
                if (flow.ok) {
                    if (body.contains("maxVecSize")) {
                        int maxVecSize = body["maxVecSize"];
                        simulator = Simulator(graph, flow, random, inputState, maxVecSize);
                    } else {
                        simulator = Simulator(graph, flow, random, inputState);
                    }
                }

            }
            
            crow::response res{j.dump()};
            res.set_header("Content-Type", "application/json");
            if (session.is_new) {
                res.add_header("Set-Cookie", "session_id=" + session.id + "; Path=/; HttpOnly; Max-Age=86400; SameSite=None; Secure");
            }
            return res;

        } catch (const std::exception& e) {
            return crow::response(500, std::string("Error: ") + e.what());
        }
    });

    CROW_ROUTE(app, "/api/qasm").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionInfo session = get_session_id(req);
        MBQC_Graph& g = get_graph_for_session(session.id);
        
        json j = g.toJson();

        crow::response res{j.dump()};
        res.set_header("Content-Type", "application/json");
        if (session.is_new) {
            res.add_header("Set-Cookie", "session_id=" + session.id + "; Path=/; HttpOnly; Max-Age=86400; SameSite=None; Secure");
        }
        return res;
    });

    CROW_ROUTE(app, "/api/qasm").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        SessionInfo session = get_session_id(req);
        MBQC_Graph& g = get_graph_for_session(session.id);
        crow::response res;

        try {
            auto j = json::parse(req.body);
            std::string qasmText;

            if (j.contains("qasm")) {
                qasmText = j["qasm"].get<std::string>();
                
                QASMParser parser("", qasmText);
                QuantumCircuit qc = parser.parse();
                g = CIRCtoMBQCGraph(qc);
                
                json reply = g.toJson();
                
                res.code = 200;
                res.set_header("Content-Type", "application/json");
                res.write(reply.dump());   

            } else {
                throw std::runtime_error("Missing field: qasm");
            }
            
        } catch (const std::exception& e) {
            json err;
            err["status"] = "error";
            err["message"] = e.what();
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(err.dump());
        }

        return res;
    });


    CROW_ROUTE(app, "/api/sim").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        SessionInfo session = get_session_id(req);
        Simulator& simulator = get_simulator_for_session(session.id);
        json j = simulator.toJson();
        
        crow::response res{j.dump()};
        res.set_header("Content-Type", "application/json");
        if (session.is_new) {
            res.add_header("Set-Cookie", "session_id=" + session.id + "; Path=/; HttpOnly; Max-Age=86400; SameSite=None; Secure");
        }
        return res;
    });

    CROW_ROUTE(app, "/api/sim").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        SessionInfo session = get_session_id(req);
        Simulator& simulator = get_simulator_for_session(session.id);
        auto j = simulator.toJson();
        
        try {
            auto body = json::parse(req.body);
            std::string qasmText;

            if (body.contains("init")) {
                bool random = body["random"];
                std::string inputState = body["input"];
                PauliFlowResult& flow = get_flow_for_session(session.id);
                if (flow.ok) {
                    MBQC_Graph& graph = get_graph_for_session(session.id);
                    if (body.contains("maxVecSize")) {
                        int maxVecSize = body["maxVecSize"];
                        simulator = Simulator(graph, flow, random, inputState, maxVecSize);
                    } else {
                        simulator = Simulator(graph, flow, random, inputState);
                    }
                }

                j = simulator.toJson();

            } else if (body.contains("measureNode")) {
                int u = body["measureNode"];
                simulator.step(u);
                j = simulator.toJson();

            } else if (body.contains("measureAll")) {
                simulator.simulateAll();
                j = simulator.toJson();
            }
            
            crow::response res{j.dump()};
            res.set_header("Content-Type", "application/json");
            if (session.is_new) {
                res.add_header("Set-Cookie", "session_id=" + session.id + "; Path=/; HttpOnly; Max-Age=86400; SameSite=None; Secure");
            }
            return res;


        } catch (const std::exception& e) {
            return crow::response(500, std::string("Error: ") + e.what());
        }
            

    });

    // Serve static frontend files
    CROW_ROUTE(app, "/<path>")
    ([](const crow::request& /*req*/, crow::response& res, std::string path) {
        const std::string dist = "./frontend/dist";

        // Sanitize path traversal
        if (path.find("..") != std::string::npos) {
            res.code = 403;
            res.end();
            return;
        }

        std::string file_path = dist + "/" + path;

        // If it's a directory or doesn't exist, fall back to index.html (SPA routing)
        if (!std::filesystem::exists(file_path) || std::filesystem::is_directory(file_path)) {
            file_path = dist + "/index.html";
        }

        auto content = read_file(file_path);
        if (!content) {
            res.code = 404;
            res.end();
            return;
        }

        res.set_header("Content-Type", get_mime_type(file_path));
        res.write(*content);
        res.end();
    });

    // Root route
    CROW_ROUTE(app, "/")
    ([]() {
        crow::response res;
        const std::string dist = "./frontend/dist";

        auto content = read_file(dist + "/index.html");

        if (content) {
            res.set_header("Content-Type", "text/html");
            res.write(*content);
        } else {
            res.code = 404;
        }

        return res;
    });

    std::thread([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::system("xdg-open http://localhost:18080");
    }).detach();

    std::cout << "Crow server starting on http://localhost:18080...\n";
    app.port(18080).multithreaded().run();
}
