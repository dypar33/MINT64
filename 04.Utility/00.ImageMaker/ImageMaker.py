
b_data = b""
k_data = b""
k64_data = b""
total_sector_offset = 5

with open('./00.BootLoader/BootLoader.bin', 'rb') as f:
    b_data = f.read()

with open('./01.Kernel32/Kernel32.bin', 'rb') as f:
    k_data = f.read()

with open('./02.Kernel64/Kernel64.bin', 'rb') as f:
    k64_data = f.read()

plus_count = 512 - (512 if (len(b_data) % 512) == 0 else len(b_data) % 512)
b_data += b"\x00" * plus_count

plus_count = 512 - (512 if (len(k_data) % 512) == 0 else len(k_data) % 512)
k_data += b"\x00" * plus_count

plus_count = 512 - (512 if (len(k64_data) % 512) == 0 else len(k64_data) % 512)
k64_data += b"\x00" * plus_count

b_data = b_data[:total_sector_offset] + (len(k_data+k64_data) // 512).to_bytes(2, 'little') + ((len(k_data) // 512)).to_bytes(2, 'little') + b_data[9:]


data = b_data + k_data + k64_data
print(len(data))

with open('./Disk.img', 'wb') as f:
    f.write(data)