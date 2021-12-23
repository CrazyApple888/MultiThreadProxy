#include <unistd.h>
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
        client->host = value;
    }
    if ("Connection" == client->h_field) {
        value = "close";
    }
    client->headers.append(client->h_field + ": " + value + "\r\n");

    return 0;
}

int onHeadersComplete(http_parser *parser) {
    auto client = (Client *) parser->data;
    client->headers.append("\r\n");
    client->isAllParsed = true;
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

    return 0;
}

Client::Client(int client_socket, bool is_debug, Cache *_cache) : logger(new Logger(is_debug)) {
    this->cache = _cache;
    this->client_socket = client_socket;
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

/**
 * Creates CacheEntity if it doesn't exist
 * @return CacheEntity* or nullptr if no enough space
 */
CacheEntity *Client::createCacheEntity() {
    if ((this->cached_data = cache->getEntity(this->url)) == nullptr) {
        this->cached_data = cache->createEntity(url);
    }

    return this->cached_data;
}

bool Client::isCacheExist() {
    if (cache->getEntity(this->url) == nullptr) {
        return false;
    } else {
        return true;
    }
}

std::string Client::getRequest() {
    std::string get_this;
    get_this.append(method);
    get_this.append(" ");
    get_this.append(url);
    get_this.append(" HTTP/1.0\r\n");
    get_this.append(headers);

    return get_this;
}

bool Client::readRequest() {
    long len;
    logger->debug(TAG, "Reading request");
    while (!isAllParsed) {
        len = read(client_socket, buffer, BUFFER_SIZE);
        if (len < 0) {
            logger->info(TAG, "Recv returned value < 0");
            return false;
        }
        if (len == 0) {
            logger->debug(TAG, "No data from server");
            break;
        }
        logger->info(TAG, "ab");
        auto parsed_len = http_parser_execute(&parser, &settings, buffer, len);
        if (parsed_len != len || 0u != parser.http_errno) {
            logger->debug(TAG, "parser errno = " + std::to_string(parser.http_errno));
            return false;
        }
    }
    logger->info(TAG, "Request is read");

    return true;
}

bool Client::readData() {
    logger->debug(TAG, "Reading data...");
    current_pos = 0;
    unsigned long read_len;

    if (cached_data == nullptr || !cached_data->isValid()) {
        cached_data->unsubscribe();
        logger->debug(TAG, "Cache is invalid");
        return false;
    }

    std::vector<char> read_data;
    while (true) {
        read_data.clear();
        cached_data->getPart(current_pos, read_len, read_data);
        //logger->debug(TAG, "Read " + std::to_string(read_len) + " bytes");

        current_pos += read_len;
        ssize_t bytes_sent = 0;
        while (bytes_sent != read_len) {
            ssize_t sent = write(client_socket, read_data.data() + bytes_sent, read_len);
            if (0 > sent) {
                logger->debug(TAG, "Send returned value < 0");
                cached_data->unsubscribe();
                return false;
            }
            if (0 == sent) {
                cached_data->unsubscribe();
                return false;
            }
            bytes_sent += sent;
        }

        if ((cached_data->isFull() && current_pos == cached_data->getRecordSize())) {
            cached_data->unsubscribe();
            logger->debug(TAG, "Cache is read, breaking while");
            break;
        }
        if (!cached_data->isValid()) {
            cached_data->unsubscribe();
            logger->debug(TAG, "Cache is invalid, breaking while");
            return false;
        }
    }

    return true;
}

Client::~Client() {
    close(client_socket);
    delete logger;
}