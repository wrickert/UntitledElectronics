#include <string>
#include <vector>

std::vector<std::string> Intro{
    "",
    " WELCOME TO THE RED TEAM VILLAGE BADGE!/ This badge will provide a brief overview on WiFi hacking as well as an entertaining game! Press A to go to the menu. The scroll wheel in the corner turns and has a click./ Enjoy!",
    " The Red Team Village badge symbolizes our mission to showcase the thrill of WiFi hacking. With an interactive and engaging approach, we aim to demonstrate the excitement and educational value of understanding wireless networks",
    " This will include a tour of vulnerabilities and defensive techniques. Our goal is to make learning cybersecurity an enjoyable experience.",
""



};

std::vector<std::string> Lessons{
    // Room 0: Intro
    " Welcome to the training floor! Each room has a lesson for you to learn about WiFi hacking. Learn well as there are bosses who do not respond well to lack of knowlege.",
    // Room 1: Terms
    " Some Terms to know: /SSID is the name of the WiFi network. WEP and WPA are types of WiFi encryption. ",
    // Room 2: Laptop Requirements
    " Aircrack-ng is a suite of tools that facilitate the assment of WiFi security. While there is a Windows version it is unsupported. Your best option is to use Linux. There is a Live-CD availible.",
    // Room 3: Monitor Mode
    // Todo reword to include Amazon
    " In order to scan and packet capture you will need a WiFi card with Monitor mode. Run 'lspci -vv' and 'lsusb -vv' to see what WiFi card your computer has. Then google the card with the term 'monitor mode'.",
    // Room 4: WiFi Security Types
    " There are many types of WiFi security but in this badge we will focus on the three main ones. /Open WiFi: no encryption, /WEP: Wired Equivalent Privacy, WPA: Wi-Fi Protected Access. ",
    // Room 5: Open WiFi
    " Open WiFi is very uncomon these days but there is one noteable exception. Hotel and College campuses are sometimes unsecured so they can offer different ways to login",
    // Room 6: Open Room Boss
    " You shouldnt see this",
    // Room 7: WEP
    " WEP encryption was invented in 1997 and it was a first attempt to secure WiFi. It had defects as you will see.",
    // Room 8: Grue Room
    " You have been eaten by a Grue",
    // Room 9: WPA Security
    " WPA was introduced in 2003. It was a signiciant advance over WEP. It uses TKIP or AES to make it harder to reverse engineer the key. It also has message integridty checks to prevent man-in-the-middle attacks",
    // Room 10: 4 Way Handshake
    " WPA uses a 4-way handshake(4wh). This is a clever method to establish an encrypted session without sharing keys until you know you can trust",
    // Room 11: Deauth Packet
    " Since the 4wh works without sharing the keys we cant fake it. /HOWEVER you can kick a user off the network then record the 4wh when they reconnect. We do that by sending a deauth packet.",
    // Room 12: WEP Encryption is obsolete
    " WEP uses a 40 or 104 bit encryption. This makes it possible to brute force",
    // Room 13: WEP Cracking via stastical methods
    " Another weakness is that each packet has a 24bit IV that is reused. If you record enough IVs you can reverse engineer the code allowing the ability to snoop on all communications.",
    // Room 14: WEP Room Boss
    " You shouldnt see this",
    // Room 15: WPA key is salted with SSID
    " The encryption key is combined with the SSID using a method called salting. This means you cant use a lookup table of common password to crack the key. This makes it important to change the Default SSID.",
    // Room 16: Haskcat
    " Once you have the 4 way handshake and the SSID you can use a table of common passwords and a program called hashcat to brute force the key. This can be a very long process even on powerful computers",
    // Room 17: Aircrack-ng
    " Aircrack-ng contains tools that allow you do deauth clients, capture the 4 way handshake, use the GPU of your computer to brute force passwords and many other handy tools for WiFi security",
    // Room 18: Rockyou.txt
    " rockout.txt is one of the most common word lists for passwords. It has a vast collection from password breaches to include millions of possible passwords. Theres a good change the WiFi key plaintext is in there.",
    // Room 19: Overview
    " We hope the previous topics were handy in giving you enough information to go with confidence and test WiFi security. Remember to only test on networks you own! Have Fun!",
    // Room 20: WPA Boss
    " You shouldnt see this."



};


std::vector<std::string> QuizQ{
    "flag:(STRINGSISGREAT)",
    "What does open encryption  refer to in WiFi networks?",
    "a) A network with no security measures & encryption",
    "b) A network secured with the latest WPA encryption",
    "c) A network using MAC filtering for access control",
    "Is an open encryption in a public place safe to use?",
    "a) Yes, its completely safe as there is no encryption",
    "b) No, it is not safe, as your data has no protection",
    "c) Yes, it is safe, you can trust people and places.",
    "Which WiFi encryption types provides smallest amount of security?",
    "a) WEP (Wired Equivalent Privacy)   ",
    "b) WPA (Wi-Fi Protected Access)   ",
    "c) Open   ",
    //WEP Quiz
    "What does WEP stand for in the context of WiFi security?",
    "a) Wi-Fi Encryption Protocol",
    "b) Wireless Enhanced Privacy",
    "c) Wired Equivalent Privacy",
    "Why is WEP considered weak and vulnerable to attacks?",
    "a) Its a complex encryption that is hard to crack.",
    "b) Short encryption key length, susceptible to brute-force",
    "c) It requires a powerful computer to decrypt.",
    "Which WiFi security protocol replaced WEP",
    "a) WPA (Wi-Fi Protected Access)",
    "b) WPS (Wi-Fi Protected Setup)",
    "c) WFM (Wi-Fi Fortress Mode)",
    //WPA Quiz
    "What is the primary purpose of WPA in WiFi networks?",
    "a) To enhance signal strength and coverage.",
    "b) To provide fast data transfer rates.",
    "c) To secure wireless communication by encrypting data.",
    "Which of the following encryption protocols is used in WPA?",
    "a) WEP (Wired Equivalent Privacy)",
    "b) TRE (Totally Real Encryption)",
    "c) TKIP (Temporal Key Integrity Protocol)",
    "What improvement did WPA bring over its predecessor, WEP?",
    "a) Longer encryption keys to prevent brute-force attacks.",
    "b) Simplified user authentication process.",
    "c) The ability to automatically connect badges"

};