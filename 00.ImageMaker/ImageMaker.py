
b_data = b""
k_data = b""
total_sector_offset = 5

with open('./00.BootLoader/BootLoader.bin', 'rb') as f:
    b_data = f.read()

with open('./01.Kernel32/Kernel32.bin', 'rb') as f:
    k_data = f.read()

plus_count = len(b_data) % 512
b_data += b"\x00" * plus_count

plus_count = len(k_data) % 512
k_data += b"\x00" * plus_count

 
b_data = b_data[:total_sector_offset] + b'\x02' + b_data[total_sector_offset+1:]


data = b_data + k_data

with open('./Disk.img', 'wb') as f:
    f.write(data)