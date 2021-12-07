#include "Client.h"

int onUrl(http_parser *parser, const char *at, size_t length) {
    auto client = (Client *) parser->data;
    if (1u != parser->method && 2u != parser->method) {
        client->getLogger()->debug(client->getTag(), "onUrl failed");
        return 1;
    }
    auto tmp = std::string(at);
    client->url.append(at, length);
    client->getLogger()->debug(client->getTag(), "URL=" + client->url);
    client->getLogger()->debug(client->getTag(), "onUrl successful");

    return 0;
}

int onHeaderField(http_parser *parser, const char *at, size_t length) {
    auto client = (Client *) parser->data;
    client->h_field = std::string(at, length);
    return 0;
}

int onHeaderValue(http_parser *parser, const char *at, size_t length) {
    auto client = (Client *) parser->data;
    auto value = std::string(at, length);
    if (client->h_field == "Host") {
        if (!client->createServerConnection(value)) {
            return 1;
        }
    }
    if ("Connection" == client->h_field) {
        value = "close";
    }
    client->headers.append(client->h_field + ": " + value + "\r\n");

    return 0;
}

int onHeadersComplete(http_parser *parser) {
    //todo rewrite me
    auto client = (Client *) parser->data;
    client->headers.append("\r\n");
    client->getLogger()->debug(client->getTag(), "All headers parsed");
    client->getLogger()->debug(client->getTag(), "method = " + std::to_string(parser->method));
    switch (parser->method) {
        case 1u:
            client->method = "GET";
            break;
        case 2u:
            client->method = "HEAD";
            break;
        default:
            return 1;
    }
    client->sendServerRequest();
    return 0;
}

Client::Client(int client_socket, bool is_debug, Proxy *proxy) : logger(new Logger(is_debug)) {
    ///-------------------Initializing fields-------------------
    this->client_socket = client_socket;
    this->proxy = proxy;
    TAG = std::string("Client " + std::to_string(client_socket));

    ///---------Initializing parser settings---------
    http_parser_settings_init(&settings);
    settings.on_url = onUrl;
    settings.on_header_field = onHeaderField;
    settings.on_header_value = onHeaderValue;
    settings.on_headers_complete = onHeadersComplete;

    ///----------------Initializing parser----------------
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;

    logger->debug(TAG, "created and initialized");
}

bool Client::execute(int event) {
    if (event & POLLIN) {
        logger->debug(TAG, "EXECUTE POLLIN, event = " + std::to_string(event | POLLIN));
        return readRequest();
    }

    if (event & POLLOUT) {
        logger->debug(TAG, "EXECUTE POLLOUT, event = " + std::to_string(event | POLLOUT));
        return readAnswer();
    }

    return false;
}

bool Client::readRequest() {
    auto len = recv(client_socket, buffer, BUFFER_SIZE, 0);
    //todo >
    if (0 >= len) {
        logger->debug(TAG, "len from recv <= 0");
        return false;
    }
    auto parsed_len = http_parser_execute(&parser, &settings, buffer, len);
    if (parsed_len != len || 0u != parser.http_errno) {
        logger->debug(TAG, "parser errno = " + std::to_string(parser.http_errno));
        return false;
    }

    return true;
}

bool Client::readAnswer() {
    logger->debug(TAG, "READING FROM CACHE");
    if (nullptr == cached_data) {
        logger->info(TAG, "BROKEN CACHE");
        proxy->disableSoc(client_socket);
        return true;
        //return false;
    }
    if (!cached_data->isValid()) {
        logger->info(TAG, "Cache invalid, shutting down");
        cached_data->unsubscribe(client_socket);
        return false;
    }
    auto cache_len = cached_data->getRecordSize();
    size_t read_len;
    if (cache_len - current_pos > BUFFER_SIZE) {
        read_len = BUFFER_SIZE;
    } else {
        read_len = cache_len - current_pos;
    }
    if (read_len == 0) {
        logger->debug(TAG, "No new data in cache");
        if (!cached_data->isFull()) {
            proxy->disableSoc(client_socket);
        } else {
            return false;
        }
        return true;
    }
    logger->debug(TAG, "Read " + std::to_string(read_len) + " bytes");
    auto data = cached_data->getPart(current_pos, read_len);
    current_pos += read_len;
    ssize_t bytes_sent = 0;
    while (bytes_sent != read_len) {
        ssize_t sent = send(client_socket, data + bytes_sent, read_len, 0);
        if (0 > sent) {
            cached_data->unsubscribe(client_socket);
            logger->debug(TAG, "Unsubing");
            return false;
        }
        if (0 == sent) {
            break;
        }
        bytes_sent += sent;
    }

    if (cached_data->isFull() && current_pos == cached_data->getRecordSize()) {
        logger->debug(TAG, "Reading completed");
        cached_data->unsubscribe(client_socket);
        return false;
    }

    logger->debug(TAG, "Completed reading from cache");

    if (!cached_data->isFull()) {
        proxy->disableSoc(client_socket);
    }

    return true;
}

bool Client::createServerConnection(const std::string &host) {
    return proxy->createServerConnection(host, this);
}

void Client::addServer(Server *ser) {
    this->server = ser;
}

void Client::sendServerRequest() {
    server->sendRequest(url.data(), headers.data(), method.data());
}

void Client::addCache(CacheEntity *cache) {
    logger->debug(TAG, "Trying to add cache");
    cached_data = cache;
    logger->debug(TAG, "Cache added");
}

Client::~Client() {
    delete logger;
}
