#if WINDOWS
using Microsoft.UI;
using Microsoft.UI.Windowing;
using Windows.Graphics;
using System.Diagnostics;
#endif


namespace RowaPickupMAUI


{
    public partial class App : Application
    {
        public App()
        {
            InitializeComponent();
            Microsoft.Maui.Handlers.WindowHandler.Mapper.AppendToMapping(nameof(IWindow), (handler, view) =>
            {
#if WINDOWS
            var mainDisplayInfo = Microsoft.Maui.Devices.DeviceDisplay.MainDisplayInfo;

            // Calculate screen size
            var screenWidth = mainDisplayInfo.Width / mainDisplayInfo.Density;
            var screenHeight = mainDisplayInfo.Height / mainDisplayInfo.Density;
            int minimumWidth=1024;
            int minimumHeight=600;
            // Check if the resolution is greater than 1024x768
            if (screenWidth > 1024 && screenHeight > 768)
            {
                // Calculate 25% of the screen size
                int desiredWidth = (int)Math.Round(screenWidth * 0.5);
                int desiredHeight = (int)Math.Round(screenHeight * 0.5);
                if (desiredWidth<minimumWidth){
                  desiredWidth=minimumWidth;
                  }
                if (desiredHeight<minimumHeight){
                  desiredHeight=minimumHeight;
                }
                var mauiWindow = handler.VirtualView;
                var nativeWindow = handler.PlatformView;
                nativeWindow.Activate();
                IntPtr windowHandle = WinRT.Interop.WindowNative.GetWindowHandle(nativeWindow);
                WindowId windowId = Microsoft.UI.Win32Interop.GetWindowIdFromWindow(windowHandle);
                AppWindow appWindow = Microsoft.UI.Windowing.AppWindow.GetFromWindowId(windowId);
                appWindow.Resize(new SizeInt32(desiredWidth, desiredHeight));
                Debug.WriteLine("w/h"+desiredWidth+"/"+desiredHeight);
            }

#endif
            });
            MainPage = new AppShell();
        }
    }
}
