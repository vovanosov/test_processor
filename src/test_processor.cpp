#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

using namespace tlm;

// Модуль памяти
class Memory : public sc_module {
public:
    // Сокет для связи с другими модулями
    tlm_utils::simple_target_socket<Memory> socket;

    // Размер памяти
    static const unsigned int MEMORY_SIZE = 256;
    sc_uint<8> mem[MEMORY_SIZE]; // Массив памяти

    SC_CTOR(Memory) : socket("socket") {
        for (int i = 0; i < MEMORY_SIZE; ++i) {
            mem[i] = (i % 3 + 5); // 5 6 7 5 6 7 5 6 7 5 6 7
        }
        socket.register_b_transport(this, &Memory::b_transport);
    }

    // Реализация метода b_transport
    void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        sc_uint<8> address = trans.get_address(); // Получаем адрес из транзакции
        unsigned char* data_ptr = trans.get_data_ptr(); // Получаем указатель на данные
        unsigned int length = trans.get_data_length(); // Получаем длину данных
        // std::cout << "Mem: request at addr=" << address << ", len=" << length << std::endl;

        if (address + length > MEMORY_SIZE) {
            trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE); // Проверка на выход за пределы памяти
            return;
        }

        if (trans.is_read()) {
            // std::cout << "Mem: reading" << std::endl;
            // Чтение из памяти
            for (unsigned int i = 0; i < length; ++i) {
                data_ptr[i] = mem[address + i]; // Чтение данных
            }
            trans.set_response_status(TLM_OK_RESPONSE); // Установка статуса ответа
        } else if (trans.is_write()) {
            // std::cout << "Mem: writing" << std::endl;
            // Запись в память
            for (unsigned int i = 0; i < length; ++i) {
                mem[address + i] = data_ptr[i]; // Запись данных
            }
            trans.set_response_status(TLM_OK_RESPONSE); // Установка статуса ответа
        } else {
            std::cout << "Mem: error" << std::endl;
            trans.set_response_status(TLM_COMMAND_ERROR_RESPONSE); // Неверная команда
        }
    }
};

// Модуль процессора
class Processor : public sc_module {
private:
    static const unsigned int REG_COUNT = 4; // Максимальное кол-во регистров
    sc_uint<8> registers[REG_COUNT]; // Регистры
public:
    tlm_utils::simple_initiator_socket<Processor> socket; // Сокет инициатора
    sc_in<uint16_t> execute_addr_in_signal;

    SC_CTOR(Processor) : socket("socket") {

        SC_THREAD(receive_process); // Создание процесса обработки программ
        sensitive << execute_addr_in_signal; // Чувствительность к сигналу
    }

    void receive_process() {
        wait();
        uint16_t pc = execute_addr_in_signal.read();
        std::cout << "OK: Received to execute from address " << pc << std::endl;
        while (true) {
            uint8_t instruction[4]; // 4 bytes: TYPE, RESULT, ARG0, ARG1
            tlm::tlm_generic_payload payload;
            sc_time delay = SC_ZERO_TIME;

            // Чтение инструкции из памяти
            payload.set_command(tlm::TLM_READ_COMMAND);
            payload.set_address(pc);
            payload.set_data_ptr(reinterpret_cast<unsigned char*>(&instruction));
            payload.set_data_length(sizeof(instruction));
            socket->b_transport(payload, delay);
            if (payload.is_response_error()) {
                std::cout << "Read instruction at addr " << pc << " returned error" << std::endl;
                return;
            }

            // Декодирование инструкции
            // std::cout << "Processing instruction " << static_cast<int>(instruction[0]) << std::endl;
            switch (instruction[0]) {
                case 0x00:
                    std::cout << "END instruction" << std::endl;
                    return;
                case 0x01: { // LOAD
                    sc_uint<8> reg = instruction[1];

                    if (reg >= REG_COUNT) {
                        std::cout << "ERR: LOAD to invalid register R" << reg << std::endl;
                        return;
                    }

                    sc_uint<8> addr = instruction[2];
                    
                    // Чтение данных из памяти
                    unsigned char data;
                    payload.set_command(tlm::TLM_READ_COMMAND);
                    payload.set_address(addr);
                    payload.set_data_ptr(&data);
                    payload.set_data_length(sizeof(data));
                    socket->b_transport(payload, delay);
                    if (payload.is_response_error()) {
                        std::cout << "ERR: LOAD addr " << addr << " returned error" << std::endl;
                        return;
                    }
                    
                    std::cout << "OK: LOAD " << static_cast<int>(data) << " to R" << reg << " from address " << addr << std::endl; 
                    registers[reg] = data;
                    break;
                }
                case 0x02: { // ADD
                    sc_uint<8> dest_reg = instruction[1];
                    if (dest_reg >= REG_COUNT) {
                        std::cout << "ERR: ADD to invalid register R" << dest_reg << std::endl;
                        return;
                    }
                    sc_uint<8> reg1 = instruction[2];
                    if (reg1 >= REG_COUNT) {
                        std::cout << "ERR: ADD: invalid register R" << reg1 << std::endl;
                        return;
                    }
                    sc_uint<8> reg2 = instruction[3];
                    if (reg2 >= REG_COUNT) {
                        std::cout << "ERR: ADD: invalid register R" << reg2 << std::endl;
                        return;
                    }
                    registers[dest_reg] = registers[reg1] + registers[reg2];
                    std::cout << "OK: ADD R" << reg1 << "+R" << reg2 << "=" << "R" << dest_reg << ", result " << registers[dest_reg] << std::endl;
                    break;
                }
                case 0x03: { // STORE
                    sc_uint<8> addr = instruction[1];
                    sc_uint<8> reg = instruction[2];
                    if (reg >= REG_COUNT) {
                        std::cout << "ERR: STORE from invalid register R" << reg << std::endl;
                        return;
                    }
                    
                    payload.set_command(tlm::TLM_WRITE_COMMAND);
                    payload.set_address(addr);
                    unsigned char data = registers[reg];
                    payload.set_data_ptr(&data);
                    socket->b_transport(payload, delay);
                    if (payload.is_response_error()) {
                        std::cout << "ERR: STORE to addr " << addr << " returned error" << std::endl;
                        return;
                    }
                    std::cout << "OK: STORE " << static_cast<int>(registers[reg]) << " from R" << reg << " to address " << addr << std::endl; 
                break;
                }
                default:
                    std::cout << "ERR: Unknown instruction " << static_cast<int>(instruction[0]) << std::endl;
                    return;
            }
    
            pc += sizeof(instruction); // Переход к следующей инструкции
        }
    }
};

// Тестовый модуль, запускающий программу. Содержит память + процессор
class TestBench : public sc_module {
public:
    sc_signal<uint16_t> signal; // Сигнал для начала работы
    Memory mem;
    Processor proc;

    SC_CTOR(TestBench): mem("memory"), proc("processor") {
        proc.execute_addr_in_signal(signal);

        proc.socket.bind(mem.socket); 
        SC_THREAD(send_program); // Создание процесса отправки
    }

    void send_program() {
        const uint8_t PROG_ADDR = 0x21;
        // Загрузка программы в память
        sc_uint<32> program[] = {
            0x01020300, // LOAD R2, [0x03]
            0x01010400, // LOAD R1, [0x04]
            0x02030102,  // ADD R3, R1, R2
            0x03050300,  // STORE [0x05], R3
            0x01000500, // LOAD R0, [0x05]
            0x00000000,  // END
        };
        mem.mem[0x03] = 10;
        mem.mem[0x04] = 15;

        for (int i = 0; i < sizeof(program) / sizeof(sc_uint<32>); ++i) {
            mem.mem[i * 4 + 0 + PROG_ADDR] = program[i].range(31, 24);
            mem.mem[i * 4 + 1 + PROG_ADDR] = program[i].range(23, 16);
            mem.mem[i * 4 + 2 + PROG_ADDR] = program[i].range(15, 8);
            mem.mem[i * 4 + 3 + PROG_ADDR] = program[i].range(7, 0);
        }
        std::cout << "TestBench: Signalling to start program" << std::endl;
        signal.write(PROG_ADDR);
    }
};

int sc_main(int argc, char* argv[]) {
    
    TestBench tb("Testbench");
    sc_start(); // Запуск симуляции

    return 0;
}
