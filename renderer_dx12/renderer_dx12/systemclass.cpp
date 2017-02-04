#include "systemclass.h"

bool System::Initialize()
{
	int screenWidth, screenHeight;
	bool result;


	// Initialize the width and height of the screen to zero before sending the variables into the function.
	screenWidth = 800;
	screenHeight = 600;

	// Initialize the windows api.
	InitializeWindows(screenWidth, screenHeight);

	// Create the input object.  This object will be used to handle reading the keyboard input from the user.
	input_ = new Input();
	if(!input_)
	{
		return false;
	}

	// Initialize the input object.
	input_->Initialize(hinstance_, hwnd_, screenWidth, screenHeight);

	// Create the graphics object.  This object will handle rendering all the graphics for this application.
	graphics_ = new Graphics();
	if(!graphics_)
	{
		return false;
	}

	// Initialize the graphics object.
	result = graphics_->Initialize(screenWidth, screenHeight, hwnd_);
	if(!result)
	{
		return false;
	}
	
	return true;
}


void System::Shutdown()
{
	// Release the graphics object.
	if(graphics_)
	{
		graphics_->Shutdown();
		delete graphics_;
		graphics_ = 0;
	}

	// Release the input object.
	if(input_)
	{
		delete input_;
		input_ = 0;
	}

	// Shutdown the window.
	ShutdownWindows();
	
	return;
}


void System::Run()
{
	MSG msg;
	bool done, result;


	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));
	
	// Loop until there is a quit message from the window or the user.
	done = false;
	while(!done)
	{
		// Handle the windows messages.
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if(msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.
			result = Frame();
			if(!result)
			{
				done = true;
			}
		}

	}

	return;
}


bool System::Frame()
{
	bool result;

	// Do the frame processing for the graphics object.
	result = graphics_->Frame();
	if(!result)
	{
		return false;
	}

	return true;
}


LRESULT CALLBACK System::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch(umsg)
	{
		// Check if a key has been pressed on the keyboard.
		case WM_KEYDOWN:
		{
		}

		// Check if a key has been released on the keyboard.
		case WM_KEYUP:
		{
		}

		// Any other messages send to the default message handler as our application won't make use of them.
		default:
		{
			return DefWindowProc(hwnd, umsg, wparam, lparam);
		}
	}
}


void System::InitializeWindows(int screen_width, int screen__height)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;


	// Get an external pointer to this object.	
	g_system_instance = this;

	// Get the instance of this application.
	hinstance_ = GetModuleHandle(NULL);

	// Give the application a name.
	application_name_ = L"Engine";

	// Setup the windows class with default settings.
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hinstance_;
	wc.hIcon		 = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm       = wc.hIcon;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = application_name_;
	wc.cbSize        = sizeof(WNDCLASSEX);
	
	// Register the window class.
	RegisterClassEx(&wc);

	// Determine the resolution of the clients desktop screen.
	screen_width  = GetSystemMetrics(SM_CXSCREEN);
	screen__height = GetSystemMetrics(SM_CYSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if(FULL_SCREEN)
	{
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth  = (unsigned long)screen_width;
		dmScreenSettings.dmPelsHeight = (unsigned long)screen__height;
		dmScreenSettings.dmBitsPerPel = 32;			
		dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else
	{
		// If windowed then set it to 800x600 resolution.
		screen_width  = 800;
		screen__height = 600;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - screen_width)  / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screen__height) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	hwnd_ = CreateWindowEx(WS_EX_APPWINDOW, application_name_, application_name_, 
						    WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
						    posX, posY, screen_width, screen__height, NULL, NULL, hinstance_, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(hwnd_, SW_SHOW);
	SetForegroundWindow(hwnd_);
	SetFocus(hwnd_);

	// Hide the mouse cursor.
	ShowCursor(false);

	return;
}


void System::ShutdownWindows()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if(FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(hwnd_);
	hwnd_ = NULL;

	// Remove the application instance.
	UnregisterClass(application_name_, hinstance_);
	hinstance_ = NULL;

	// Release the pointer to this class.
	g_system_instance = NULL;

	return;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch(umessage)
	{
		// Check if the window is being destroyed.
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);		
			return 0;
		}

		// All other messages pass to the message handler in the system class.
		default:
		{
			return g_system_instance->MessageHandler(hwnd, umessage, wparam, lparam);
		}
	}
}