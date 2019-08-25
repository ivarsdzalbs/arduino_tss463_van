#include "tss463_van.h"
#include <util/delay.h>

int ExtractBits(const int value, const int numberOfBits, const int pos)
{
    return (((1 << numberOfBits) - 1) & (value >> (pos - 1)));
}

void TSS463_VAN::tss_init() 
{
    motorolla_mode();
    _delay_ms(10);

    // disable all channels
    for (uint8_t i = 0; i < CHANNELS; i++) {
        disable_channel(i);
        _delay_ms(10);
    }

    #pragma region Line Control Register (0x00) documentation
    /*
    Line Control Register (0x00) - Read/Write
    +-----+------+-----+-----+----+---+-------+-------+
    | CD3 |  CD2 | CD1 | CD0 | PC | 0 | IVTX0 | IVRX0 |
    +-----+------+-----+-----+----+---+-------+-------+
    - Default value after reset: 0×00
    - Reserved: Bit 2, this bit must not be set by the user; a 0 must always be written to this bit.
    
    CD[3:0]:Clock Divider They control the VAN Bus rate through a Baud Rate generator according to the formula below: 
      Clock Divider 0010 - 0x02 ( TSCLK = XTAL1 / n * 16) or ( TSCLK = 16000000 / 2 * 16) or 500000
    PC: Pulsed Code 
      One: The TSS463C will transmit and receive data using the pulsed coding mode (i.e optical or radio link mode). The use of this mode implies communication via the RxD0 input and the non-functionality of the diagnosis system.
      Zero: (default at reset) The TSS463C will transmit and receive data using the Enhanced Manchester code. (RxD0, RxD1, RxD2 used).
    IVTX: Invert TxD output
    IVRX: Invert RxD inputs The user can invert the logical levels used on either the TxD output or the RxD inputs in order to adapt to different line drivers and receivers.
        One: A one on either of these bits will invert the respective signals.
        Zero: (default at reset) The TSS463C will set TxD to recessive state in Idle mode and consider the bus free (recessive states on RxD inputs).
    */  
    #pragma endregion

    register_set(LINECONTROL, 0x20); // Clock Divider 0010 - 0x02 ( TSCLK = XTAL1 / n * 16) or ( TSCLK = 16000000 / 2 * 16) or 500000
    _delay_ms(10);

    #pragma region Transmit Control Register (0x01) documentation
    /*
    Transmit Control Register (0x01)
    +-----+------+-----+-----+------+------+------+----+
    | MR3 |  MR2 | MR1 | MR0 | VER2 | VER1 | VER0 | MT |
    +-----+------+-----+-----+------+------+------+----+
    MR[3:0]: Maximum Retries These bits allow the user to control the amount of retries the circuit will perform if any errors occurred during transmission.
    VER[2:0] = 001 DLC Version after reset. These bits must not be set by user. 001 must always be written to these bits.
    MT: Module Type The three different module types are supported (see “VAN Frame” on page 15):
        One : The TSS463C is at once an autonomous module (Rank 0), a synchronous access module (Rank 1) or a slave module (Rank 16). 
            Page 15: The Autonomous module, that is a bus master. It can transmit Start of Frame (SOF) sequences, it can initiate data transfers and can receive messages.
        Zero: The TSS463C is at once an synchronous access module (Rank 1) or a slave module (Rank 16).
            Page 15: The Synchronous access module. It cannot transmit SOF sequences, but it can initiate data transfers and can receive messages.
                        The Slave module, that can only transmit using an in-frame mechanism and can receive messages.
    */
    #pragma endregion

    register_set(TRANSMITCONTROL, B00000011); // MR: 0011 (Maximum Retries = 0x01) VER 001 fixed
    _delay_ms(10);


    // Enable TSS Interrupts
    uint8_t intEnable = 0x80; // Default value reset: 1xx0 0000
    intEnable |= ( 1 << ROKE) | ( 1 << ROKE);
    /*
    Interrupt Reset Register (0x0B) - Write only
    +------+---+---+-----+------+-----+------+-------+
    | RSTR | 0 | 0 | TER | TOKR | RER | ROKR | RNOKR |
    +------+---+---+-----+------+-----+------+-------+
    Reserved bit: 5 and 6. This bit must not be set by user; a zero must always be written to this bit
    RSTR: Reset Interrupt Reset
      One: Status flag reset.
      Zero: Status flag unchanged.
    TER: Transmit Error Status Flag  Reset
      One: Status flag reset.
      Zero: Status flag unchanged.
    TOKR: Transmit OK Status Flag Reset
      One: Status flag reset.
      Zero: Status flag unchanged
    RER: Receive Error Status Flag Reset
      One: Status flag reset.
      Zero: Status flag unchanged.
    ROKR: Receive “with RAK” OK Status Flag Reset
      One: Status flag reset.
      Zero: Status flag unchanged.
    RNOKR: Receive “with no RAK” OK Status Flag Reset
      One: Status flag reset.
      Zero: Status flag unchanged.    
    */

    #pragma region Interrupt Enable Register (0x0A) documentation
    /*
    Interrupt Enable Register (0x0A) - Read/Write
    +---+---+---+-----+------+-----+------+-------+
    | 1 | X | X | TEE | TOKE | REE | ROKE | RNOKE |
    +---+---+---+-----+------+-----+------+-------+
    Default value reset: 1xx0 0000
    Note: On reset, the Reset Interrupt Enable bit is set to 1 instead of 0, as is the general rule.
    TEE: Transmit Error Enable
      One: IT enabled.
      Zero: IT disabled.
    TOKE: Transmission OK Enable
      One: IT enabled.
      Zero: IT disabled.
    REE: Reception Error Enable
      One: IT enabled.
      Zero: IT disabled.
    ROKE: Reception “with RAK” OK Enable
      One: IT enabled.
      Zero: IT disabled.
    RNOKE: Reception “with no RAK” OK Enable
      One: IT enabled.
      Zero: IT disabled.
    */
    #pragma endregion

    register_set(INTERRUPTENABLE, intEnable);
    _delay_ms(10);


    #pragma region Command Register (0x03) documentation
    /*
    Command Register (0x03) - Write only register.
    +------+-------+------+------+------+---+---+------+
    | GRES | SLEEP | IDLE | ACTI | REAR | 0 | 0 | MSDC |
    +------+-------+------+------+------+---+---+------+
    Reserved: Bit 1, 2. These bits must not be set by the user; a zero must always be written to these bit.
    If the circuit is operating at low bitrates, there might be a considerable delay between the writing of this register and the performing of the actual command (worst
    case 6 timeslots). The user is therefore recommended to verify, by reading the Line  Status Register (0x04), that the commands have been performed.
    
    GRES: General Reset The Reset circuit command bit performs, if set, exactly as if the external reset pin was asserted. This command bit has its own auto-reset circuitry.
        One: Reset active
        Zero: Reset inactive
    SLEEP: Sleep command (Section “Sleep Command”, page 51). If the user sets the Sleep bit, the circuit will enter sleep mode. When the circuit is in sleep mode, all non-user registers are setup to minimize power consumption.
            Read/write accesses to the TSS463C via the SPI/SCI interface are impossible, the oscillator is stopped. To exit from this mode the user must apply either an hardware reset (external RESET pin) either an asynchronous software reset (via the SPI/SCI interface).
        One: Sleep active
        Zero: Sleep inactive
    IDLE: Idle command (Section “Idle and Activate Commands”, page 51). If the user sets the Idle bit, the circuit will enter idle mode. In idle mode the oscillator will operate, but the TSS463C will not transmit or receive anything on the bus, and the TxD output will be in tri-state
        One: Idle active
        Zero: Idle inactive
    ACTI: Activate command (Section “Idle and Activate Commands”, page 51). The Activate command will put the circuit in the active mode, i.e it will transmit and receive normally on the bus. When the circuit is in activate mode the TxD tri-state output is enabled.
        One: Activate active
        Zero: Activate inactive
    REAR: Re-Arbitrate command. This command will, after the current attempt, reset the retry counter and re-arbitrate the messages to be transmitted in order to find the highest priority message to transmit.
        One: Re-arbitrate active
        Zero: Re-arbitrate inactive
    MSDC: Manual System Diagnosis Clock. Rather than using the SDC divider described in Section “SDC Signal (Synchronous Diagnosis Clock)” , the user can use the manual SDC command to generate a SDC pulse for the diagnosis system. This MSDC pulse should be high at least 2 timeslot clocks.      
    */
    #pragma endregion

    register_set(COMMANDREGISTER, B10000);  // ACTI - activate line
    _delay_ms(10);
    error = 0;

    #pragma region Fill Message DATA RAM area with 0x00
    uint8_t zeroarray[128];
    memset(zeroarray, 0, sizeof(zeroarray));
    registers_set(GETMAIL(0), zeroarray, 128);
    #pragma endregion
}

uint8_t TSS463_VAN::spi_transfer(uint8_t data)
{
    uint8_t res;

    SPDR = data;
    while (!(SPSR & (1 << SPIF))) {};
    res = SPDR;

    return res;
}

void TSS463_VAN::register_set(uint8_t address, uint8_t value)
{
    uint8_t res = 0;

    TSS463_SELECT();

    _delay_ms(4);
    //At the beginning of a transmission over the serial interface, the first byte is the address of the TSS463C register to be accessed
    res = spi_transfer(address);
    if (res != ADDR_ANSW)
        error++;
    _delay_ms(8);
    //The next byte transmitted is the control byte that determines the direction of the communication
    res = spi_transfer(WRITE);
    if (res != CMD_ANSW)
        error++;
    _delay_ms(15);
    spi_transfer(value);
    _delay_ms(12);

    TSS463_UNSELECT();
}

void TSS463_VAN::registers_set(const uint8_t address, const uint8_t values[], const uint8_t count)
{
    uint8_t i;
    uint8_t res = 0;

    TSS463_SELECT();

    _delay_ms(4);
    //At the beginning of a transmission over the serial interface, the first byte is the address of the TSS463C register to be accessed
    res = spi_transfer(address);
    if ( res != ADDR_ANSW)
    error++;
    _delay_ms( 8);
    //The next byte transmitted is the control byte that determines the direction of the communication
    res = spi_transfer(WRITE);
    if (res != CMD_ANSW)
    error++;
    _delay_ms(15);

    for (i = 0; i < count; i++)
    {
        spi_transfer(values[i]);
    }
    _delay_ms(12);

    TSS463_UNSELECT();
}

uint8_t TSS463_VAN::register_get(uint8_t address)
{
    uint8_t value;

    TSS463_SELECT();

    _delay_ms(4);
    //At the beginning of a transmission over the serial interface, the first byte is the address of the TSS463C register to be accessed
    value = spi_transfer(address);
    if (value != ADDR_ANSW)
        error++;
    _delay_ms(8);
    //The next byte transmitted is the control byte that determines the direction of the communication
    value = spi_transfer(READ);
    if (value != CMD_ANSW)
        error++;
    _delay_ms(15);
    //When the master (CPU) conducts a read, it sends an address byte, a control byte and dummy characters (�0xFF�for instance) on its MOSI line
    value = spi_transfer(0xff);
    _delay_ms(12);

    TSS463_UNSELECT();

    return value;
}

uint8_t TSS463_VAN::registers_get(const uint8_t address, volatile uint8_t values[], const uint8_t count)
{
    uint8_t value;

    TSS463_SELECT();

    _delay_ms(4);
    //At the beginning of a transmission over the serial interface, the first byte is the address of the TSS463C register to be accessed
    value = spi_transfer(address);
    if (value != ADDR_ANSW)
        error++;
    _delay_ms(8);
    //The next byte transmitted is the control byte that determines the direction of the communication
    value = spi_transfer(READ);
    if (value != CMD_ANSW)
        error++;
    _delay_ms(15);
    //When the master (CPU) conducts a read, it sends an address byte, a control byte and dummy characters (�0xFF�for instance) on its MOSI line

    // TSS463 has auto-increment of address-pointer
    for (uint8_t i = 0; i < count; i++)
    {
        values[i] = spi_transfer(0xff);
    }
    _delay_ms(12);

    TSS463_UNSELECT();

    return value;
}

void TSS463_VAN::motorolla_mode()
{
    uint8_t value;

    TSS463_SELECT();

    _delay_ms(4);
    value = spi_transfer(MOTOROLA_MODE);
    if (value != ADDR_ANSW)
        error++;
    _delay_ms(8);
    value = spi_transfer(MOTOROLA_MODE);
    if (value != CMD_ANSW)
        error++;
    _delay_ms(12);

    TSS463_UNSELECT();
}

void TSS463_VAN::disable_channel(const uint8_t channelId)
{
    register_set(CHANNEL_ADDR(channelId) + 0, 0x00);  //  ID_TAG 9C4 - Radio Control
    register_set(CHANNEL_ADDR(channelId) + 1, 0x00);  //  ID_TAG, RNW = 0, RTR = 1
    register_set(CHANNEL_ADDR(channelId) + 2, 0x00);  //  MESS_PTR 0 ( 0x80 + 0) - MAILBOX ADDRESS
    register_set(CHANNEL_ADDR(channelId) + 3, 0x0F);  //  M_L [4:0] = 0x1F Frame with 30 DATA bytes , CHER = 0, CHTx = 0, CHRx = 0
    register_set(CHANNEL_ADDR(channelId) + 6, 0x00);  //  ID_MASK 9C4
    register_set(CHANNEL_ADDR(channelId) + 7, 0x00);  //  ID_MASK
}

void TSS463_VAN::setup_channel(const uint8_t channelId, const uint8_t id1, const uint8_t id2, const uint8_t id2AndCommand, const uint8_t messagePointer, const uint8_t lengthAndStatus)
{
    /*
    :...............:........:.......:.......:.......:.......:.......:.......:.......:.......:
    :Reg. Name      : Offset : Bit 7 : Bit 6 : Bit 5 : Bit 4 : Bit 3 : Bit 2 : Bit 1 : Bit 0 :
    :...............:........:.......:.......:.......:.......:.......:.......:.......:.......:
    :ID_MASK        :  0x07  :           ID_M [3:0]          :   x   :   x   :   x   :   x   :
    :...............:........:...............................:.......:.......:.......:.......:
    :ID_MASK        :  0x06  :                        ID_M [11:4]                            :
    :...............:........:.......:.......:.......:.......:.......:.......:.......:.......:
    :(No register)  :  0x05  :   x   :   x   :   x   :   x   :   x   :   x   :   x   :   x   :
    :...............:........:.......:.......:.......:.......:.......:.......:.......:.......:
    :(No register)  :  0x04  :   x   :   x   :   x   :   x   :   x   :   x   :   x   :   x   :
    :...............:........:.......:.......:.......:.......:.......:.......:.......:.......:
    :MESS_L / STA   :  0x03  :                   M_L [4:0]           : CHER  :  CHTx :  CHRx :  Message length and status register
    :...............:........:.......:...............................:.......:.......:.......:
    :MESS_PTR       :  0x02  : DRACK :                      M_P [6:0]                        :  DRAK : Message Address (DRAK : Disable RAK : Used in 'Spy Mode')
    :...............:........:.......:.......................:.......:.......:.......:.......:
    :ID_TAG / CMD   :  0x01  :           ID_T [3:0]          : EXT   :  RAK  :  RNW  :  RTR  :  ID_TAG:COM
    :...............:........:...............................:.......:.......:.......:.......:
    :ID_TAG         :  0x00  :                         ID_T [11:4]                           :
    :...............:........:...............................................................:
    */
    register_set(CHANNEL_ADDR(channelId) + 0, id1);             //  ID_TAG
    register_set(CHANNEL_ADDR(channelId) + 1, id2AndCommand);   //  ID_TAG, RNW = 0, RTR = 1
    register_set(CHANNEL_ADDR(channelId) + 2, messagePointer);  //  MESS_PTR 0 ( 0x80 + 0) - MAILBOX ADDRESS
    register_set(CHANNEL_ADDR(channelId) + 3, lengthAndStatus); //  M_L [4:0] = 0x1F Frame with 30 DATA bytes , CHER = 0, CHTx = 0, CHRx = 0
    register_set(CHANNEL_ADDR(channelId) + 6, id1);             //  ID_MASK 9C4
    register_set(CHANNEL_ADDR(channelId) + 7, id2);             //  ID_MASK
}

/*
        Transmit Message structure: (Page 44)
......................................................
:                    : RNW : RTR : CHTx :    CHRx    :
:....................:.....:.....:......:............:
: Initial setup      :   0 :   1 :    0 : Don't care :
: After transmission :   0 :   0 :    1 : Unchanged  :
:....................:.....:.....:......:............:
*/
//Page 44 contains how the RNW, RTR, CHTx, CHRx registers should be configured to send a message on a channel
void TSS463_VAN::set_channel_for_transmit_message(uint8_t channelId, uint8_t id1, uint8_t id2, const uint8_t values[], uint8_t messageLength, uint8_t requireAck)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 0;
    id2Command.data.RTR = 0;
    id2Command.data.EXT = 1;
    id2Command.data.RAK = requireAck;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 0;
    messagePointer.data.M_P = 127-(messageLength+1); //TODO hardcoded memory location for sending messages !!!

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHTx = 0;
    lengthAndStatus.data.M_L  = messageLength+1;

    uint8_t addressOfDataToSendOnVAN = GETMAIL(messagePointer.data.M_P+1);
    registers_set(addressOfDataToSendOnVAN, values, messageLength);

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

/*
        Receive Message structure: (Page 44)
......................................................
:                    : RNW : RTR :    CHTx    : CHRx :
:....................:.....:.....:............:......:
: Initial setup      :   0 :   1 : Don't care :    0 :
: After transmission :   0 :   1 : Unchanged  :    1 :
:....................:.....:.....:......:............:
*/
void TSS463_VAN::set_channel_for_receive_message(uint8_t channelId, uint8_t id1, uint8_t id2, uint8_t messageLength)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 0;
    id2Command.data.RTR = 1;
    id2Command.data.EXT = 1;
    id2Command.data.RAK = 0;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 1;
    messagePointer.data.M_P = channelId*30;

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHRx = 0;
    lengthAndStatus.data.CHTx = 0;
    lengthAndStatus.data.M_L  = messageLength+1;

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

/*
 Reply Request Message without transmission: (Page 44)
......................................................
:                    : RNW : RTR :    CHTx    : CHRx :
:....................:.....:.....:............:......:
: Initial setup      :   1 :   1 : Don't care :    0 :
: After transmission :   1 :   1 : Unchanged  :    1 :
:....................:.....:.....:......:............:
*/
void TSS463_VAN::set_channel_for_reply_request_message_without_transmission(uint8_t channelId, uint8_t id1, uint8_t id2, uint8_t messageLength)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 1;
    id2Command.data.RTR = 1;
    id2Command.data.EXT = 1;//should be 1
    id2Command.data.RAK = 0;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 1;
    messagePointer.data.M_P = channelId*30;

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHRx = 0;
    lengthAndStatus.data.CHTx = 0;
    lengthAndStatus.data.M_L  = messageLength+1;

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

/*
        Reply Request Message: (Page 44)
:.....................................:.....:.....:......:............:
:                                     : RNW : RTR :    CHTx    : CHRx :
:.....................................:.....:.....:......:............:
: Initial setup                       :   1 :   1 : 0          :    0 :
: After transmission(wait for reply)  :   1 :   1 : 1          :    1 :
: After reception(of reply)           :   1 :   1 : 1          :    1 :
:.....................................:.....:.....:......:............:
*/
void TSS463_VAN::set_channel_for_reply_request_message(uint8_t channelId, uint8_t id1, uint8_t id2, uint8_t messageLength)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 1;
    id2Command.data.RTR = 1;
    id2Command.data.EXT = 1;
    id2Command.data.RAK = 0;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 1;
    messagePointer.data.M_P = channelId * 30;

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHRx = 0;
    lengthAndStatus.data.CHTx = 0;
    lengthAndStatus.data.M_L = messageLength + 1;

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

/*
        Reply Request Message: (Page 45)
:.....................................:.....:.....:......:............:
:                                     : RNW : RTR :    CHTx    : CHRx :
:.....................................:.....:.....:......:............:
: Initial setup                       :   1 :   0 : 0          :    0 :
: After transmission                  :   1 :   0 : 1          :    1 :
:.....................................:.....:.....:......:............:
*/
void TSS463_VAN::set_channel_for_immediate_reply_message(uint8_t channelId, uint8_t id1, uint8_t id2, const uint8_t values[], uint8_t messageLength)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 1;
    id2Command.data.RTR = 0;
    id2Command.data.EXT = 1;
    id2Command.data.RAK = 0;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 0;
    messagePointer.data.M_P = channelId * 30;

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHRx = 0;
    lengthAndStatus.data.CHTx = 0;
    lengthAndStatus.data.M_L = messageLength + 1;

    uint8_t addressOfDataToSendOnVAN = GETMAIL(messagePointer.data.M_P + 1);
    registers_set(addressOfDataToSendOnVAN, values, messageLength);

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

/*
        Deferred Reply Message: (Page 45)
:.....................................:.....:.....:......:............:
:                                     : RNW : RTR :    CHTx    : CHRx :
:.....................................:.....:.....:......:............:
: Initial setup                       :   1 :   0 : 0          :    1 :
: After transmission                  :   1 :   0 : 1          :    1 :
:.....................................:.....:.....:......:............:
*/
void TSS463_VAN::set_channel_for_deferred_reply_message(uint8_t channelId, uint8_t id1, uint8_t id2, const uint8_t values[], uint8_t messageLength)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 1;
    id2Command.data.RTR = 0;
    id2Command.data.EXT = 1;
    id2Command.data.RAK = 0;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 1;
    messagePointer.data.M_P = channelId * 30;

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHRx = 1;
    lengthAndStatus.data.CHTx = 0;
    lengthAndStatus.data.M_L = messageLength + 1;

    uint8_t addressOfDataToSendOnVAN = GETMAIL(messagePointer.data.M_P + 1);
    registers_set(addressOfDataToSendOnVAN, values, messageLength);

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

/*
        Reply Request Detection Message: (Page 45)
:.....................................:.....:.....:......:............:
:                                     : RNW : RTR :    CHTx    : CHRx :
:.....................................:.....:.....:......:............:
: Initial setup                       :   1 :   0 : 1          :    0 :
: After transmission                  :   1 :   0 : 1          :    1 :
:.....................................:.....:.....:......:............:
*/
void TSS463_VAN::set_channel_for_reply_request_detection_message(uint8_t channelId, uint8_t id1, uint8_t id2, uint8_t messageLength)
{
    //Page38
    Id2AndCommandRegister id2Command;
    memset(&id2Command, 0, sizeof(id2Command));
    id2Command.data.ID = id2;
    id2Command.data.RNW = 1;
    id2Command.data.RTR = 0;
    id2Command.data.EXT = 1;
    id2Command.data.RAK = 0;

    //Page38
    MessagePointerRegister messagePointer;
    memset(&messagePointer, 0, sizeof(messagePointer));
    messagePointer.data.DRAK = 1;
    messagePointer.data.M_P = channelId * 30;

    //Page 39
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.data.CHRx = 0;
    lengthAndStatus.data.CHTx = 1;
    lengthAndStatus.data.M_L = messageLength + 1;

    setup_channel(channelId, id1, id2, id2Command.Value, messagePointer.Value, lengthAndStatus.Value);
}

MessageLengthAndStatusRegister TSS463_VAN::message_available(uint8_t channelId)
{
    MessageLengthAndStatusRegister lengthAndStatus;
    memset(&lengthAndStatus, 0, sizeof(lengthAndStatus));
    lengthAndStatus.Value = register_get(CHANNEL_ADDR(channelId) + 3);

    return lengthAndStatus;
}

uint8_t TSS463_VAN::readMsgBuf(const uint8_t channelId, uint8_t*len, uint8_t buf[])
{
    uint8_t id1 = register_get(CHANNEL_ADDR(channelId) + 0);
    uint8_t id2 = register_get(CHANNEL_ADDR(channelId) + 1);
    uint8_t messageStatusLocationInMemory = ExtractBits(register_get(CHANNEL_ADDR(channelId) + 2), 7, 1); //exclude DRAK bit

    /*
        Message Status(Pointed by : Message Pointer Register)
        ..............................................................
        : RRAK : RRNW : RRTR : RM_L4 : RM_L3 : RM_L2 : RM_L1 : RM_L0 :
        :......:......:......:.......:.......:.......:.......:.......:
        RRAK: Received RAK Bit:  This bit is the RAK bit coming from the COM field of the received frame.
        RRNW: Received RNW Bit:  This bit is the RNW bit coming from the COM field of the received frame.
        RRTR: Received RTR Bit:  This bit is the RTR bit coming from the COM field of the received frame.
        RM_L[4:0]: Message Length of the Received Frame
                   If the DATA field of the received frame included DATA0 to DATAn, RM_L[4:0] = n+1, even if the reserved length (Message Length and Status Register) is larger.
    */

    uint8_t messageStatusByte = register_get(GETMAIL(messageStatusLocationInMemory));
    uint8_t messageLength = ExtractBits(messageStatusByte, 5, 1);
    //messageLength += 2; // + 2 to include CRC in array
    uint8_t command = ExtractBits(messageStatusByte, 3, 6);

    uint8_t currentData[messageLength];
    uint8_t addr = GETMAIL(channelId * 30 + 1);

    uint8_t d = registers_get(addr, currentData, messageLength);

    buf[0] = id1;
    buf[1] = id2;
    //buf[2] = messageStatusByte; //include message status byte in array

    for (uint8_t i = 0; i < messageLength; i++)
    {
        buf[i + 2] = currentData[i];
        //buf[i+3] = currentData[i]; //include message status byte in array
    }

    //*len = messageLength + 3; //include message status byte in array
    *len = messageLength + 2;
}

uint8_t TSS463_VAN::getlastChannel() {
    uint8_t lms = register_get(LASTMESSAGESTATUS);
    uint8_t channelId = ExtractBits(lms, 3, 1);
    return channelId;
}

void TSS463_VAN::begin()
{
    //TIMSK0 = 0; //Disable Timer 0 interrupt - causes delay() to stop working
    DDRB |= (1 << DDB3) | (1 << DDB5) | (1 << DDB2) | (1 << DDB1) | (1 << DDB0); // pin 3,5,2,1,0 as outputs rest left in their original state

    PORTB |= (0 << PINB0);
    PORTB |= (1 << PINB2);
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA);
    byte clr = SPSR;
    clr = SPDR;
    (void)clr;

    _delay_ms(10);
    tss_init();
}

TSS463_VAN::TSS463_VAN(uint8_t _CS) {
    SPICS = _CS;
    pinMode(SPICS, OUTPUT);
    TSS463_UNSELECT();
}

extern TSS463_VAN VAN;
/* EOF */