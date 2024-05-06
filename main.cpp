#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <boost/asio.hpp>
#include <boost/process.hpp>

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    int port = 0;
    std::vector<std::string> syncMachines;
    
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " -p <port> <sync_machines>..." << std::endl;
        return 1;
    }

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-p") {
            if (i + 1 < argc) {
                port = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: Missing port number after -p" << std::endl;
                return 1;
            }
        } else {
            syncMachines.push_back(argv[i]);
        }
    }

    if (port == 0 || syncMachines.empty()) {
        std::cerr << "Error: Both port and sync machines' IPs must be specified." << std::endl;
        return 1;
    }

    // Function to handle wl-paste watch and POST data to server
    auto handle_wl_paste = [&](const std::string& url) {
        std::string command = "wl-paste --watch sh -c \"curl -X POST " + url + " -d '$(wl-paste)'\"";
        system(command.c_str());
    };

    // Function to handle HTTP requests and write data to clipboard using wl-copy
    auto handle_request = [&](const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
        std::string request_body;

        // Read the data from the socket
        boost::asio::streambuf buffer;
        boost::asio::read_until(*socket, buffer, "\r\n\r\n");
        std::istream input(&buffer);
        std::getline(input, request_body, '\0');

        // Write the data to the clipboard using wl-copy
        boost::process::ipstream error_stream;
        boost::process::opstream input_stream;
        boost::process::child wl_copy("wl-copy", boost::process::std_in < input_stream, boost::process::std_err > error_stream);

        input_stream << request_body;
        input_stream.close();

        wl_copy.wait();
        int exit_code = wl_copy.exit_code();

        // Send an HTTP response back to the client
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\n" + std::string("done");
        boost::asio::write(*socket, boost::asio::buffer(response));
        socket->close();
    };

    // Function to start the HTTP server
    auto start_server = [&] {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

        while (true) {
            auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
            acceptor.accept(*socket);
            std::thread(handle_request, socket).detach();
        }
    };

    // Create the URL using the first sync machine's IP
    std::string url = "http://" + syncMachines[0] + ":" + std::to_string(port);

    // Start wl-paste watch in a separate thread
    std::thread wl_paste_thread(handle_wl_paste, url);

    // Start the HTTP server
    start_server();

    // Wait for wl-paste thread to finish
    wl_paste_thread.join();

    return 0;
}
