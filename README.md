# Introduction

**GrammarChecker** is a utility for instant grammar correction in any text field via keyboard shortcut, powered by Large Language Models (LLM).
![chrome_ROjd9ShCmE](https://github.com/user-attachments/assets/6724e3b6-2858-413b-976f-e78d640c39b2)

# How It Works
Currently, it utilizes the OpenAI API, requiring an API key to function. Simply select the text you want to check and trigger the assigned shortcut. Grammar Checker can be automatically launched on startup and run in the system tray. The main window provides a user-friendly interface where you can assign any specific keyboard shortcut for triggering grammar checks or adjust the prompt requirements to align with your specific writing needs, such as translating text into different language or something else.

![GrammarChecker](https://github.com/user-attachments/assets/b69ea8cc-3627-4b3a-a819-a2a3e0284fa8)
# Dependencies
- [liboai](https://github.com/D7EAD/liboai)
- [nlohmann-json](https://github.com/nlohmann/json)
- [curl](https://github.com/curl/curl)

# Building
Requires Qt 6.8 or newer. Buildable on Windows with Qt Creator.

# License
GrammarChecker is licensed under the GPL-3.0 license, see LICENSE.txt for more information.
