#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <string>
#include <thread>

std::string user_name;

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;



class perftest {
public:
    typedef perftest type;

    perftest() {
        m_endpoint.set_access_channels(websocketpp::log::alevel::none);
        m_endpoint.set_error_channels(websocketpp::log::elevel::none);

        m_endpoint.init_asio();

        m_endpoint.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
        m_endpoint.set_open_handler(bind(&type::on_open, this, ::_1));
        m_endpoint.set_fail_handler(bind(&type::on_fail, this, ::_1));
    }

    void start(std::string uri) {
        websocketpp::lib::error_code ec;
        con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
            return;
        }

        m_endpoint.connect(con);

        m_endpoint.run();
    }

    context_ptr on_tls_init(websocketpp::connection_hdl) {
        context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);

        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                boost::asio::ssl::context::no_sslv2 |
                boost::asio::ssl::context::no_sslv3 |
                boost::asio::ssl::context::single_dh_use);
        }
        catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
        return ctx;
    }

    void on_fail(websocketpp::connection_hdl hdl) {
        con = m_endpoint.get_con_from_hdl(hdl);

        std::cout << "Fail handler" << std::endl;
        std::cout << con->get_state() << std::endl;
        std::cout << con->get_local_close_code() << std::endl;
        std::cout << con->get_local_close_reason() << std::endl;
        std::cout << con->get_remote_close_code() << std::endl;
        std::cout << con->get_remote_close_reason() << std::endl;
        std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
    }

    void on_open(websocketpp::connection_hdl hdl) {
        std::cout << "[SERVER]: Connected to the chat\n";
        std::cout << "[SERVER]: You can use chat commands: /setname [name], /pm [id] [msg], /bot [msg]\n";
        m_endpoint.send(hdl, "/setname " + user_name, websocketpp::frame::opcode::text);
    }

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
        std::cout << msg->get_payload(); 
        //m_endpoint.close(hdl, websocketpp::close::status::going_away, "");
    }

    void stop() {
        m_endpoint.stop();
    }

    void send_message(std::string msg) {
        m_endpoint.send(con->get_handle(), msg, websocketpp::frame::opcode::text);
    }

private:
    client::connection_ptr con;
    client m_endpoint;
};

int main(int argc, char* argv[]) {
    std::cout << "Enter you name: ";
    std::getline(std::cin, user_name);

    perftest endpoint;

    std::thread* thr = new std::thread([&endpoint]() {
        std::string uri = "ws://127.0.0.1:9999";

        endpoint.start(uri);
    });

    std::string input;
    do {
        getline(std::cin, input);
        endpoint.send_message(input);
    } while (input.compare("exit"));

    endpoint.stop();
    thr->join();

    return 0;
}