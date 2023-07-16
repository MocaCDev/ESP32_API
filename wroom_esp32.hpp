#ifndef WROOM_ESP32_API
#define WROOM_ESP32_API
#include <Arduino.h>

namespace WROOM_ESP32
{

/* OAI - Output And Input. 
 * These pins can be used for input and output.
 * */
enum class OAI_PINS
{
    OAI_PIN_NONE = 0x0,
    D2 = 0x02,
    D4 = 0x04,
    D5 = 0x05,
    D12 = 0x0C,
    D13 = 0x0D,
    D14 = 0x0E,
    D15 = 0x0F,
    D18 = 0x12,
    D19 = 0x13,
    D21 = 0x15,
    D22 = 0x16,
    D23 = 0x17,
    D25 = 0x19,
    D26 = 0x1A,
    D27 = 0x1B,
};

enum access
{
    WRITE,
    READ,
    READ_AND_WRITE,
    NONE
};

struct OAI_PINS_DATA
{
    const OAI_PINS pins[0xF] = {
        OAI_PINS::D2, OAI_PINS::D4, OAI_PINS::D5, 
        OAI_PINS::D12, OAI_PINS::D13, OAI_PINS::D14,
        OAI_PINS::D15, OAI_PINS::D18, OAI_PINS::D19,
        OAI_PINS::D21, OAI_PINS::D22, OAI_PINS::D23,
        OAI_PINS::D25, OAI_PINS::D26, OAI_PINS::D27
    };

    enum access pin_access[0xF] = {
        NONE, NONE, NONE,
        NONE, NONE, NONE,
        NONE, NONE, NONE,
        NONE, NONE, NONE,
        NONE, NONE, NONE
    };

    /* If true, the pins and there access are set-and-stone.
     * If false, the API will expect the user to continue to change the access.
     * If false, and a method is used that needs to write/read a pin, this will automatically be set to true.
     * */
    bool pin_access_finalized = false;
};

/* Data about action ocurred with a pin.
 * This will store data for read-in data and write data.
 * */
struct OAI_PIN_ACTION_DATA
{
    OAI_PINS pin;
    enum access action;
    bool data;
};

class OAI_PINS_API
{
private:
    struct OAI_PINS_DATA *pin_data = nullptr;
    struct OAI_PIN_ACTION_DATA pin_action_data = {
        .pin = OAI_PINS::OAI_PIN_NONE,
        .action = NONE,
        .data = false
    };

    enum access last_pin_access()
    { return pin_action_data.action; }

    OAI_PINS last_pin()
    { return pin_action_data.pin; }

    bool last_pin_data()
    { return pin_action_data.data; }

    void set_pin_action(OAI_PINS pin, enum access action)
    {
        switch(action)
        {
            case WRITE: pinMode((unsigned char)pin, OUTPUT);break;
            case READ: pinMode((unsigned char)pin, INPUT);break;
            case READ_AND_WRITE: pinMode((unsigned char)pin, INPUT);break;
            default: break;
        }
        return;
    }

    enum access pin_set_action(OAI_PINS pin)
    {
        unsigned char index = 0;
        for(; index < 0xF; index++)
            if(pin_data->pins[index] == pin) break;
        
        return pin_data->pin_access[index];
    }

public:
    OAI_PINS_API()
    {
        pin_data = new OAI_PINS_DATA;
    }

    void pin_read(OAI_PINS pin)
    {
        if(!pin_data->pin_access_finalized)
            finalize_pins();
        
        pin_action_data.pin = pin;
        pin_action_data.action = READ;
        pin_action_data.data = digitalRead((unsigned char)pin);
    }

    /* Return the value read in from the pin. */
    signed char pin_read_data()
    {
        if(last_pin_access() == READ)
            return last_pin_data();
        
        return -1;
    }

    void pin_write(OAI_PINS pin, bool data)
    {
        if(!pin_data->pin_access_finalized)
            finalize_pins();

        pin_action_data.pin = pin;
        pin_action_data.action = WRITE;
        pin_action_data.data = data;
        digitalWrite((unsigned char)pin, data);
    }

    /* pin_perform - perform an action `action` on pin `pin`.
     *      OAI_PINS pin - the pin to perform `action` on.
     *      enum access action - the action to perform on `pin`.
     *          If `action` is `READ_AND_WRITE`, it will perform both actions.
     *      bool data - if `action` is `WRITE`, `data` will be passed to `pin_write`.
     * */
    void pin_perform(OAI_PINS pin, enum access action, bool data)
    {
        if(last_pin() != OAI_PINS::OAI_PIN_NONE)
        {
            if(pin_set_action(pin) == READ_AND_WRITE)
            {
                if((last_pin() == pin && last_pin_access() != action) || (last_pin() != pin))
                {
                    if(action == READ_AND_WRITE) set_pin_action(pin, WRITE);
                    else set_pin_action(pin, action);
                }
                
                goto do_rest;
            }

            if(pin_set_action(pin) != action) return;
        }
        
        do_rest:
        switch(action)
        {
            case READ: pin_read(pin);break;
            case WRITE: pin_write(pin, data);break;
            case READ_AND_WRITE: {
                pin_write(pin, data);
                
                /* Delay to allow any sort of input to come in. */
                delay(3000);
                set_pin_action(pin, READ);
                break;
            }
            default: break;
        }
        return;
    }

    void set_pin_access(OAI_PINS pin, enum access pin_access)
    {
        if(pin_data->pin_access_finalized)
        {
            Serial.println("Cannot set pin access after the pins have been finalized");
            return;
        }

        /* Make sure `pin_access` is not `NONE`.
         * If it is, just return.
         * */
        if(pin_access == NONE)
            return;

        unsigned char index = 0;
        for(; index < 0xF; index++)
            if(pin_data->pins[index] == pin) break;
        
        pin_data->pin_access[index] = pin_access;
        set_pin_action(pin, pin_access);
    }

    void finalize_pins()
    {
        /* Make sure there were pins that got initialized. */
        unsigned char i = 0;
        for(; i < 0xF; i++)
            if(pin_data->pin_access[i] != NONE) break;
        
        /* If no pins got initialized, error. */
        if(i == 0xF && pin_data->pin_access[i] == NONE)
        {
            Serial.println("No pins got initialized.");
            exit(EXIT_FAILURE);
        }

        pin_data->pin_access_finalized = true;
    }

    ~OAI_PINS_API()
    {
        if(pin_data) delete pin_data;

        pin_data = nullptr;
    }
};

/* Input pins only. */
enum class INP_PINS
{
    D32 = 0x20,
    D33 = 0x21,
    D34 = 0x22,
    D35 = 0x23
};

}

#endif
