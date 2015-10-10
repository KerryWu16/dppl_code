/*
 * DubinsPathPlanner tool for solving the Dubins Traveling Salesperson problem.
 *
 * Copyright (C) 2014-2015 DubinsPathPlanner.
 * Created by David Goodman <dagoodma@gmail.com>
 * Redistribution and use of this file is allowed according to the terms of the MIT license.
 * For details see the LICENSE file distributed with DubinsPathPlanner.
 */

#include <memory>
#include <cxxopts.h>
#include <stacktrace.h>

#include <solveDTSP.h>

#include <dpp/basic/Logger.h>
#include <dpp/basic/Util.h>

/** 
 * Solve a DTSP scenario. This is an entry point interface.
 * @param G     To hold the graph of the solution.
 * @param GA    Attributes of solution graph.
 * @param x     Initial heading.
 * @param r     Vehicle turn radius.
 * @param Tour  List of nodes to hold result
 * @param Edges List of edges to hold result
 * @param Headings Node array of headings to hold result
 * @param returnToInitial Return to the initial configuration at the end of tour.
 * @param alg   Algorithm for solving DTSP. See dpp::DubinsVehiclePathPlanner::PlanningAlgorithm
 * @return Success or failure
 */
 int solveDTSP(ogdf::Graph &G, ogdf::GraphAttributes &GA, double x, double r,
    ogdf::List<ogdf::node> &Tour, ogdf::List<ogdf::edge> &Edges, NodeArray<double> &Headings,
    double &cost, bool returnToInitial, PlanningAlgorithm alg) {

    dpp::DubinsVehiclePathPlanner p;

    // Waypoints from given Graph
    p.algorithm(alg);
    p.addWaypoints(G, GA);

    // Setup path planner
    p.initialHeading(x);
    p.turnRadius(r);
    p.returnToInitial(returnToInitial);
    p.solve();

    // Get the results
    //p.copySolution(G, GA, Edges, Headings, Tour, cost);
    //G = *(p.graphPtr());
    //GA = p.graphAttributes();
    Edges = p.edges();
    Headings = p.headings();
    Tour = p.tour();
    cost = p.cost();

    //cout << "Solved " << G.numberOfNodes() << " point tour with cost " << cost << "." << endl;

    return SUCCESS;
}

/** Main Entry Point
 * The solution is a tour, which is an ordered list of waypoints,
 * saved inside a TSPlib file. The total cost, and edges are printed.
 * 
 * usage: solveDTSP [OPTIONS] <inputGMLFile> <startHeading> <turnRadius>
 * @param inputGMLFile  input GML file to read the problem from
 * @param startHeading  a starting heading in radians [0,2*pi)
 * @param turnRadius    a turning radius in radians
 * @return An exit code (0==SUCCESS)
 *
 * @options
 * -d, --debug
 *     Enables debug mode in logging module for extra information.
 * -h, --help
 *     Prints a help message and exits.
 * -a, --algorithm=<PlanningAlgorithmName>
 *     Sets the planning algorithm ("nearest", "alternating", "randomized").
 * -r,--return
 *     Whether to return to initial configuration.
 */
int main(int argc, char *argv[]) {

    // Setup stack traces for debugging
    char const *program_name = argv[0];
    #ifdef DPP_DEBUG
    set_signal_handler(program_name);
    #endif

    // Input arguments
    string inputFilename;
    double x, r;
    bool noReturn = false;
    bool debug = false;

    dpp::DubinsVehiclePathPlanner p;

    // Option parsing
    cxxopts::Options options(program_name,
        " gml_file initial_heading turn_radius");
    try {
        options.add_options()
            ("d,debug", "Enable debugging messages",cxxopts::value<bool>(debug))
            ("h,help", "Print this message")
            ("a,algorithm", "Algorithm for DTSP (nearest,alternating,randomized =default)",
                cxxopts::value<std::string>()->default_value("randomized"), "DTSP_ALGORITHM")
            ("r,noreturn", "Disables returning to initial configuration", cxxopts::value<bool>(noReturn))
            //("", "x is the initial heading")
            //("", "r is the turn radius of the Dubins vehicle");
            // TODO hide these:
            ("inputGMLFile", "Input GML file to read graph from", cxxopts::value<std::string>(), "INPUT_GML_FILE")
            ("initialHeading", "Initial heading orientation", cxxopts::value<double>(), "INITIAL_HEADING")
            ("turnRadius", "Dubins vehicle turning radius", cxxopts::value<double>(), "TURN_RADIUS");


        // Exit with help message if they didn't provide enough arguments
        if (argc < 4) {
            std::cout << program_name << ": " << " Expected at least 3 arguments." << std::endl;
            std::cout << options.help();
            return FAILURE;
        }

        options.parse_positional({"inputGMLFile", "initialHeading", "turnRadius"});
        options.parse(argc, argv);

        // Read arguments
        std::cout << "Debug=" << debug << ", noreturn=" << noReturn << std::endl;

        inputFilename =  options["inputGMLFile"].as<std::string>();
        x = options["initialHeading"].as<double>();
        r = options["turnRadius"].as<double>();

        // Set debug mode
        if (debug) {
            dpp::Logger *log = dpp::Logger::Instance();
            log->level(dpp::Logger::Level::LL_DEBUG);
        }

        // Set desired algorithm
        if (options.count("algorithm")) {
            std::string algName = options["algorithm"].as<std::string>();
            if (algName.compare("nearest") == 0) {
                p.algorithm(PlanningAlgorithm::NEAREST_NEIGHBOR);
            }
            else if (algName.compare("alternating") == 0) {
                p.algorithm(PlanningAlgorithm::ALTERNATING);
            }
            else if (algName.compare("randomized") == 0) {
                p.algorithm(PlanningAlgorithm::RANDOMIZED);
            }
            else {
                std::cout << program_name << ": " << "Invalid algorithm \'" << algName << "\'." << std::endl;
                std::cout << options.help();
                return FAILURE;
                //exit(1);
            }
        } // (options.count("algorithm"))

        // Print help and exit if help option was set
        if (options.count("help")) {
            std::cout << options.help() << std::endl;
            return SUCCESS;
            //exit(SUCCESS);
        }

    } catch (const cxxopts::OptionException& e) {
        std::cout << program_name << " error parsing options: " << e.what() << std::endl;
        return FAILURE;
    } // try

    // Set up planner
    p.addWaypoints(inputFilename);
    p.initialHeading(x);
    p.turnRadius(r);
    p.returnToInitial(!noReturn);

    p.solve();

    // Results. Must use graph pointer.
    ogdf::Graph *G = p.graphPtr();
    ogdf::GraphAttributes GA = p.graphAttributes();
    ogdf::List<ogdf::edge> E = p.edges();
    ogdf::List<ogdf::node> Tour = p.tour();
    ogdf::NodeArray<double> Headings = p.headings();
    double cost = p.cost();

    // Print headings and edge list
    ogdf::node u;
    std::cout << "Solved " << G->numberOfNodes() << " point tour with cost " << cost << "." << std::endl;
    dpp::printHeadings(*G, GA, Headings);
    dpp::printEdges(*G, GA, E);

    return SUCCESS;
}