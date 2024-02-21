#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <windows.h>

std::atomic<bool> clickEnabled(false);

void clickLeftMouseButtonUp()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));

    std::cout << "Up" << std::endl;
}

void clickLeftMouseButtonDown()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    std::cout << "Down" << std::endl;
}

// Function to handle mouse input events
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_MBUTTONDOWN)
    {
        clickEnabled.store(!clickEnabled);
        std::cout << "Middle mouse button down! clickEnabled:" << clickEnabled.load() << std::endl;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void messageLoop(std::atomic<bool> &stopFlag, std::atomic<bool> &clickEnabled)
{
    constexpr int messagesPerSecond = 20;
    constexpr std::chrono::nanoseconds interval(1000000000 / messagesPerSecond); // 1 second = 1,000,000,000 nanoseconds
    bool UpOrDown = true;

    auto nextTime = std::chrono::steady_clock::now() + interval;
    int messageCount = 0;

    while (!stopFlag.load())
    {
        if (clickEnabled)
        {
            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTime)
            {
                if (UpOrDown)
                {
                    clickLeftMouseButtonDown();
                }
                else
                {
                    clickLeftMouseButtonUp();
                }
                UpOrDown = !UpOrDown;
                std::cout << "messageCount: " << ++messageCount << std::endl;
                nextTime += interval;
            }
        }
        else
        {
            if (!UpOrDown)
            {
                clickLeftMouseButtonUp();
                UpOrDown = !UpOrDown;
            }
        }
    }
}

int main()
{
    std::atomic<bool> stopFlag(false);

    // Start the message loop in a separate thread
    std::thread messageThread(messageLoop, std::ref(stopFlag), std::ref(clickEnabled));

    // Install a mouse hook to capture mouse events
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    if (mouseHook == NULL)
    {
        std::cerr << "Failed to install mouse hook!" << std::endl;
        return 1;
    }

    // Message loop to keep the program running
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook the mouse hook before exiting
    UnhookWindowsHookEx(mouseHook);

    // Set the stop flag to true to signal the message loop to stop
    stopFlag.store(true);

    // Wait for the message loop thread to finish
    messageThread.join();

    return 0;
}
