echo "Bluetooth Clock: 60"
echo "     Raw Data: 10 D0 00 C1 9E 81 3F AB 74 72 97 86 5D 64 0C 01 2A C2 CB E7 09"
echo -n "  Output Data: "
./btwhite 60 10 D0 00 C1 9E 81 3F AB 74 72 97 86 5D 64 0C 01 2A C2 CB E7 09

echo "Expected Data: 6F 0C 00 8B C1 04 C9 37 EE B3 41 43 19 44 55 DF CB 4D D0 42 A6"
