#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

enum JsonTag {
    JSON_NUMBER = 0,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL = 0xF
};

struct JsonNode;

#define JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
#define JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
#define JSON_VALUE_TAG_MASK 0xF
#define JSON_VALUE_TAG_SHIFT 47

struct JsonValue {
    JsonTag tag;
    union
    {
        uint64_t ival;
        double fval;
    } u;

    JsonValue(double x) {
        tag = JSON_NUMBER;
        u.fval = x;
    }
    JsonValue(JsonTag _tag = JSON_NULL, void *_payload = nullptr) {
        tag = _tag;
        u.ival = (uint64_t)_payload;
    }
    bool isDouble() const {
        return tag == JSON_NUMBER;
    }
    JsonTag getTag() const {
        return tag;
    }
    uint64_t getPayload() const {
        return u.ival;
    }
    double toNumber() const {
        assert(getTag() == JSON_NUMBER);
        return u.fval;
    }
    char *toString() const {
        assert(getTag() == JSON_STRING);
        return (char *)getPayload();
    }
    JsonNode *toNode() const {
        assert(getTag() == JSON_ARRAY || getTag() == JSON_OBJECT);
        return (JsonNode *)getPayload();
    }
};

struct JsonNode {
    JsonValue value;
    JsonNode *next;
    char *key;
};

struct JsonIterator {
    JsonNode *p;

    void operator++() {
        p = p->next;
    }
    bool operator!=(const JsonIterator &x) const {
        return p != x.p;
    }
    JsonNode *operator*() const {
        return p;
    }
    JsonNode *operator->() const {
        return p;
    }
};

inline JsonIterator begin(JsonValue o) {
    return JsonIterator{o.toNode()};
}
inline JsonIterator end(JsonValue) {
    return JsonIterator{nullptr};
}

inline JsonNode *getFirstChildByKey(JsonValue o, const char *key)
{
    for (auto i : o)
        if (strcmp(i->key, key) == 0)
            return i;
    return NULL;
}

#define JSON_ERRNO_MAP(XX)                           \
    XX(OK, "ok")                                     \
    XX(BAD_NUMBER, "bad number")                     \
    XX(BAD_STRING, "bad string")                     \
    XX(BAD_IDENTIFIER, "bad identifier")             \
    XX(STACK_OVERFLOW, "stack overflow")             \
    XX(STACK_UNDERFLOW, "stack underflow")           \
    XX(MISMATCH_BRACKET, "mismatch bracket")         \
    XX(UNEXPECTED_CHARACTER, "unexpected character") \
    XX(UNQUOTED_KEY, "unquoted key")                 \
    XX(BREAKING_BAD, "breaking bad")

enum JsonErrno {
#define XX(no, str) JSON_##no,
    JSON_ERRNO_MAP(XX)
#undef XX
};

const char *jsonStrError(int err);

class JsonAllocator {
    struct Zone {
        Zone *next;
        size_t used;
    } *head = nullptr;

public:
    JsonAllocator() = default;
    JsonAllocator(const JsonAllocator &) = delete;
    JsonAllocator &operator=(const JsonAllocator &) = delete;
    JsonAllocator(JsonAllocator &&x) : head(x.head) {
        x.head = nullptr;
    }
    JsonAllocator &operator=(JsonAllocator &&x) {
        head = x.head;
        x.head = nullptr;
        return *this;
    }
    ~JsonAllocator() {
        deallocate();
    }
    void *allocate(size_t size);
    void deallocate();
};

int jsonParse(char *str, char **endptr, JsonValue *value, JsonAllocator &allocator);
