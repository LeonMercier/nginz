print("std::string body = \"", end="")
size = 300
behavior = ["scroll", "slide", "alternate"]
for i in range(size):
    print("<marquee", " behavior=", behavior[i % 3], " scrollamount=10>", end="")
    for _ in range(i): print("_", end="")
    print("Hello World", end="")
    for _ in range(size - i): print("_", end="")
    print("</marquee><br>", end="")
print("\";")
print("<marquee
  direction=down
  width=250
  height=200
  behavior=alternate
  style=border:solid>
  <marquee behavior=alternate>Hello, World!</marquee>
</marquee>
