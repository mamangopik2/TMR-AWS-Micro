import asyncio
import random
import crcmod
from pymodbus.server import StartAsyncSerialServer
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext, ModbusSequentialDataBlock


# -------------------------------
# Modbus CRC16 Calculation
# -------------------------------
def compute_modbus_crc(data: bytes) -> bytes:
    crc16 = crcmod.predefined.mkCrcFun('modbus')
    crc_value = crc16(data)
    return crc_value.to_bytes(2, byteorder='little')  # Modbus RTU = little-endian


# -------------------------------
# Print Master Request Bytes
# -------------------------------
def print_master_request():
    frame = bytes([0x01, 0x03, 0x00, 0x00, 0x00, 0x02])  # Read 2 regs from address 0
    crc = compute_modbus_crc(frame)
    full_frame = frame + crc
    print(f"[Master Request Bytes] {full_frame.hex().upper()}")


# -------------------------------
# Modbus RTU Slave Data Store
# -------------------------------
store = ModbusSlaveContext(
    hr=ModbusSequentialDataBlock(0, [random.randint(0, 65535) for _ in range(20)])
)
context = ModbusServerContext(slaves=store, single=True)


# -------------------------------
# Periodic Register Update
# -------------------------------
async def update_registers():
    while True:
        values = [random.randint(0, 65535) for _ in range(20)]
        store.setValues(3, 0, values)
        print(f"[Update] Holding Registers: {values}")
        await asyncio.sleep(5)


# -------------------------------
# Main Async Server
# -------------------------------
async def main():
    print_master_request()
    await asyncio.gather(
        StartAsyncSerialServer(
            context=context,
            port="COM3",     # ← CHANGE THIS to your serial port
            baudrate=9600,
            parity="N",
            stopbits=1,
            bytesize=8,
            framer="rtu"
        ),
        update_registers()
    )


# -------------------------------
# Run Script
# -------------------------------
if __name__ == "__main__":
    asyncio.run(main())
