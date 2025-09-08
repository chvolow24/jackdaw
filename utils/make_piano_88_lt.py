opening = """<Layout>
  <x>REL 0</x>
  <y>REL 0</y>
  <w>SCALE 1.0</w>
  <h>SCALE 1.0</h>
  <children>"""
closing = """  </children>
</Layout>"""
white_template ="""  <Layout name="{notename}" type="NORMAL">
    <x>REL 0</x>
    <y>SCALE {yscale}</y>
    <w>SCALE 1.0</w>
    <h>SCALE {hscale}</h>
    <children>
    </children>
  </Layout>"""
black_template ="""  <Layout name="{notename}" type="NORMAL">
    <x>REL 0</x>
    <y>SCALE {yscale}</y>
    <w>SCALE 0.6</w>
    <h>SCALE {hscale}</h>
    <children>
    </children>
  </Layout>"""

# OCTAVES=7
# NUM_KEYS=12 * OCTAVES + 1
# NUM_WHITE_KEYS=7 * OCTAVES + 1
NUM_KEYS=88
NUM_WHITE_KEYS=52

notenames = {
    0:'c',
    1:'d',
    2:'e',
    3:'f',
    4:'g',
    5:'a',
    6:'b'
}


white_i = 0;
flat = 0;
scale_degree = 0
octave = 8

whites_minus_first_key= 1.0 - (1/NUM_KEYS) + 0.006

print(opening)
for i in range(NUM_KEYS):
    if flat == 0: # is white
        white = white_template.format(
            notename = notenames[scale_degree] + str(octave),
            yscale = 0 if white_i == 0 else 1/NUM_KEYS + (white_i - 1) * whites_minus_first_key / (NUM_WHITE_KEYS - 1),
            # yscale = 0 if white_i == 0 else 1/NUM_KEYS + (white_i - 1) * (NUM_KEYS - 1)/NUM_KEYS/(NUM_WHITE_KEYS - 1),
            hscale = 1/NUM_KEYS if i==0 else (whites_minus_first_key+0.05)/(NUM_WHITE_KEYS - 1));
        print(white)
        white_i+=1
    else: # is black
        black = black_template.format(notename = notenames[scale_degree] + "b" + str(octave), yscale = i/NUM_KEYS, hscale = 1/NUM_KEYS);
        print(black)
        
    
    if flat == 0:
        if scale_degree in [6,5,4,2,1]:
            flat = 1
        else:
            scale_degree-=1
    else:
        flat = 0
        scale_degree-=1
    if scale_degree == -1:
        scale_degree = 6
        octave-=1
print(closing)
    

# for i in range(50):
#     print(f"{i}: start at {i/50}; black spans ({i/50 - 1/200} - {i/50 + 1/200})")
