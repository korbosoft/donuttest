import re

def boobulate():
    font = []
    current_bytes = []

    # 1. Load the font header
    with open("console_font_8x16.c", 'r', encoding='cp437') as f:
        for line in f:
            clean = re.sub(r'/\*.*?\*/', '', line)
            finds = re.findall(r'0x[0-9a-fA-F]{2}', clean)
            for h in finds:
                current_bytes.append(int(h, 16))
                if len(current_bytes) == 16:
                    font.append(list(current_bytes))
                    current_bytes = []

    densities = []
    for i in range(32, 126):
        on_pixels = sum(bin(byte).count('1') for byte in font[i])
        densities.append({'char': chr(i), 'density': on_pixels})

    densities.sort(key=lambda x: x['density'])

    ramp = []
    num_chars = len(densities)
    for i in range(32):
        index = int(i * (num_chars - 1) / 31)
        ramp.append(densities[index]['char'])

    print("boobulation has completed")
    print('char boobs[] = "' + "".join(ramp) + '";')

if __name__ == "__main__":
    boobulate()
