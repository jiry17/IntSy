//
// Created by pro on 2019/10/15.
//

#ifndef FLASHFILL_TYPE_H
#define FLASHFILL_TYPE_H

#include <string>
#include <vector>

#include "config.h"
#include "non_terminal.h"
#include "z3++.h"

class Value {
protected:
    virtual void getValue(void* res) const = 0;

public:
    virtual Type getType() const = 0;
    virtual Value* copy() const = 0;
    virtual z3::expr getZ3Expr(z3::context& ctx) const = 0;
    virtual std::string toString() const = 0;
    virtual ~Value() = default;

    int getInt() const {
#ifdef DEBUG
        if (getType() != TINT) {
            assert(0);
        }
#endif
        int res; getValue(&res); return res;
    }

    bool getBool() const {
#ifdef DEBUG
        if (getType() != TBOOL) {
            assert(0);
        }
#endif
        bool res; getValue(&res); return res;
    }

    std::string getString() const {
#ifdef DEBUG
        if (getType() != TSTRING) {
            assert(0);
        }
#endif
        std::string res; getValue(&res); return res;
    }
};

class IntValue: public Value {
    int value;

protected:
    virtual void getValue(void* res) const {
        auto* pos = static_cast<int*>(res);
        *pos = value;
    }

public:
    virtual Type getType() const {return TINT;}
    virtual IntValue* copy() const {return new IntValue(value);}
    virtual z3::expr getZ3Expr(z3::context& ctx) const {return ctx.int_val(value);}
    virtual std::string toString() const {return std::to_string(value);}
    virtual ~IntValue() = default;
    IntValue(int _value): value(_value) {}
};

class BoolValue: public Value {
    bool value;

protected:
    virtual void getValue(void* res) const {
        auto* pos = static_cast<bool*>(res);
        *pos = value;
    }

public:
    virtual Type getType() const {return TBOOL;}
    virtual BoolValue* copy() const {return new BoolValue(value);}
    virtual z3::expr getZ3Expr(z3::context& ctx) const {return ctx.bool_val(value);}
    virtual std::string toString() const {return value ? "true" : "false";}
    BoolValue(bool _value): value(_value) {}
    virtual ~BoolValue() = default;
};

class StringValue: public Value {
    std::string value;
    virtual void getValue(void* res) const {
        auto* pos = static_cast<std::string*>(res);
        *pos = value;
    }

public:
    virtual Type getType() const {return TSTRING;}
    virtual StringValue* copy() const {return new StringValue(value);}
    StringValue(std::string _value): value(_value) {}
    virtual ~StringValue() = default;
    virtual std::string toString() const {return value;}
    virtual z3::expr getZ3Expr(z3::context& ctx) const {return ctx.string_val(value);}
};

class Data;

typedef std::vector<Data> DataList;

class Data {
public:
    Value* value;
    Type getType() const {return value->getType();}
    int getInt() const {return value->getInt();}
    bool getBool() const {return value->getBool();}
    std::string getString() const {return value->getString();}
    z3::expr getZ3Expr(z3::context& ctx) const {return value->getZ3Expr(ctx);}
    std::string toString() const {return value->toString();}

    Data(Value* _value): value(_value) {}
    Data(const Data& _data): value(_data.value->copy()) {}
    ~Data() {delete value;}


    bool operator == (const Data& data) const {
        switch(getType()) {
            case TINT: return getInt() == data.getInt();
            case TBOOL: return getBool() == data.getBool();
            case TSTRING: return getString() == data.getString();
            default: assert(0);
        }
    }

    bool operator < (const Data& data) const {
        switch(getType()) {
            case TINT: return getInt() < data.getInt();
            case TBOOL: return getBool() < data.getBool();
            case TSTRING: return getString() < data.getString();
            default: assert(0);
        }
    }

    static DataList getAllValue(Type type) {
        switch (type) {
            case TINT: {
                DataList result;
                int temp_max = KIntMax * 2;
                for (int i = -temp_max; i <= temp_max; ++i) {
                    result.push_back(Data(new IntValue(i)));
                }
                return result;
            }
            case TBOOL: {
                DataList result = {Data(new BoolValue(true)), Data(new BoolValue(false))};
                return result;
            }
            default:
                assert(0);
        }
    }
};


#endif //FLASHFILL_TYPE_H
