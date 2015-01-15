//
// Includes and preprocessor directives.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// For attached console.
#include <stdio.h>
#include <io.h>
#include <fcntl.h>


//
// Function prototypes.
//

void UserActive( void );
bool CreateConsole( void );
bool InstallHooks( void );
LRESULT CALLBACK KeyboardProc( int, WPARAM, LPARAM );
LRESULT CALLBACK MouseProc( int, WPARAM, LPARAM );


//
// Global variables.
//

HHOOK mouse_hook;
HHOOK keyboard_hook;

// This is only global so that the keyboard
// hook can send a message to it easily.
HWND hwnd;


//
// Functions.
//


//
// This function is run whenever there is
// user activity on the system. You could,
// for example, record the time of the event
// and use it elsewhere in your application.
//
void UserActivity( void )
{
	// Do something.
}


//
// Attaches a console window to a regular Win32 app.
//
bool CreateConsole( void )
{
	if( NULL == AllocConsole( ) )
	{
		wchar_t error[64];
		_snwprintf_s( error, sizeof error, L"Failed to attach console window. Error code: %u", GetLastError( ) );
		MessageBox( NULL, error, L"Error", MB_ICONEXCLAMATION | MB_OK );
		return false;
	}

	int crt = _open_osfhandle( (long)GetStdHandle( STD_OUTPUT_HANDLE ), _O_TEXT );
	*stdout = *_fdopen( crt, "w" );
	setvbuf( stdout, NULL, _IONBF, NULL );

	return true;
}


//
// Install the keyboard and mouse hooks.
// Returns true if successful and false if
// either hook fails to install.
//
bool InstallHooks( void )
{
	//
	// Install the keyboard hook.
	//

	keyboard_hook = SetWindowsHookEx( WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle( NULL ), NULL );

	if( NULL == keyboard_hook )
	{
		printf("Keyboard hook failed to install. error code: %u\n", GetLastError( ) );
		return false;
	}

	printf("Keyboard hook installed successfully.\n");

	//
	// Install the mouse hook.
	//

	mouse_hook = SetWindowsHookEx( WH_MOUSE_LL, (HOOKPROC)MouseProc, GetModuleHandle( NULL ), NULL );

	if( NULL == mouse_hook )
	{
		printf("Mouse hook failed to install. Error code: %u", GetLastError( ) );
		UnhookWindowsHookEx( keyboard_hook );
		return false;
	}

	printf("Mouse hook installed successfully.\n");

	return true;
}


//
// Our keyboard proc. See the following URL for details:
// http://msdn.microsoft.com/en-us/library/ms644985%28VS.85%29.aspx
// Pressing Escape will allow the main program loop to exit
// and uninstall our keyboard and mouse hooks.
//
LRESULT CALLBACK KeyboardProc( int code, WPARAM wParam, LPARAM lParam )
{
	// Skip processing if the code is less than zero.
	// See "Return Value" at the MSDN URL above.
	if( code < 0 )
		return CallNextHookEx( keyboard_hook, code, wParam, lParam );

	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
	if( p->vkCode == VK_ESCAPE )
	{
		PostMessage( hwnd, WM_CLOSE, NULL, NULL );
	}
	
	printf("keyboard activity!!!\n");
	UserActivity( );
	
	return CallNextHookEx( keyboard_hook, code, wParam, lParam );
}


//
// Our mouse proc. See the following URL for details:
// http://msdn.microsoft.com/en-us/library/ms644986%28VS.85%29.aspx
//
LRESULT CALLBACK MouseProc( int code, WPARAM wParam, LPARAM lParam )
{
	// Skip processing if the code is less than zero.
	// See "Return Value" at the MSDN URL above.
	if( code < 0 )
		return CallNextHookEx( mouse_hook, code, wParam, lParam );

	switch( wParam )
	{
		case WM_MOUSEMOVE:

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:

		// ... and so forth. Add all the mouse events
		// you consider to be user activity.

		{
			printf("mouse activity!!!\n");
			UserActivity( );
		}
	}

	return CallNextHookEx( mouse_hook, code, wParam, lParam );
}


//
// Our hidden window's message pumper.
// Does nothing interesting except handle
// unhooking when the window is destroyed.
//
LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_CLOSE:
			DestroyWindow( hwnd );

			UnhookWindowsHookEx( keyboard_hook ); 
			UnhookWindowsHookEx( mouse_hook );

			FreeConsole( );
		break;

		case WM_DESTROY:
			PostQuitMessage( NULL );
		break;

		default:
			return DefWindowProc( hwnd, msg, wParam, lParam );
	}

	return 0;
}


//
// The entrypoint.
//
// We create a console window that prints anything
// sent to stdout and then install our hooks.
// After that, we create an invisible window and
// sit in a message pumping loop until the user
// presses their Escape key.
//
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	if( false == CreateConsole( ) )
		return 1;

	if( false == InstallHooks() )
	{
		FreeConsole( );
		return 1;
	}

	//
	// Create a plain Win32 window we don't even show.
	// A wndproc needs to be running for our hooks not
	// to interfere with normal system operation.
	//

	const wchar_t class_name[] = L"whatever";
	WNDCLASSEX wc;
	MSG msg;

	wc.cbSize        = sizeof( WNDCLASSEX );
	wc.lpszClassName = class_name;
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = hInstance;
	wc.cbClsExtra    = NULL;
	wc.cbWndExtra    = NULL;
	wc.style         = NULL;
	wc.hIconSm       = NULL;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;

	if( NULL == RegisterClassEx( &wc ) )
	{
		wchar_t error[64];
		_snwprintf_s( error, sizeof error, L"Failed to register window class. Error code: %u", GetLastError( ) );
		MessageBox( NULL, error, L"Error", MB_ICONEXCLAMATION | MB_OK );
		return 1;
	}

	hwnd = CreateWindowEx( NULL, class_name, class_name, NULL,
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, NULL, NULL, hInstance, NULL);

	if( NULL == hwnd )
	{
		wchar_t error[64];
		_snwprintf_s( error, sizeof error, L"Failed to create window. Error code: %u", GetLastError( ) );
		MessageBox( NULL, error, L"Error", MB_ICONEXCLAMATION | MB_OK );
		return 1;
	}

	while( GetMessage( &msg, NULL, NULL, NULL ) > 0 )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return msg.wParam;
}

