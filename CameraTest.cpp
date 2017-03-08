#include <pylon/PylonIncludes.h>
#include <pylon/ImagePersistence.h>
#include <unistd.h>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Before using any pylon methods, the pylon runtime must be initialized.
    PylonInitialize();

    try
    {

        // Get the transport layer factory.
        CTlFactory& tlFactory = CTlFactory::GetInstance();

        // Get all attached devices and exit application if
        // two cameras aren't found
        DeviceInfoList_t devices;
        if ( tlFactory.EnumerateDevices(devices) != 2 )
        {
            cout << "Found " << devices.size() << " cameras." << endl;
            throw RUNTIME_EXCEPTION( "Did not find exactly two cameras.");
        }
        CInstantCamera upper, lower;
        upper.Attach(tlFactory.CreateDevice( devices[0] ) ); upper.Open();
        lower.Attach(tlFactory.CreateDevice( devices[1] ) ); lower.Open();

        CGrabResultPtr ptrGrabResult;
        char filename[100]; int n;

        CImageFormatConverter fc;
	fc.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage image;

        upper.StartGrabbing(3); n=0;
        usleep(1000000);
        lower.StartGrabbing(3);
        while ( upper.IsGrabbing() ) {
            upper.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
            if ( ptrGrabResult->GrabSucceeded()) {
                snprintf(filename, 100, "images/Upper%03d.png", n++);
                cout << "Writing image to " << filename << endl;
                // Convert to OpenCV Mat
		fc.Convert(image, ptrGrabResult);
		Mat cimg(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(),
			CV_8UC3, (uint8_t *)image.GetBuffer());
		imwrite(filename, cimg);
                //CImagePersistence::Save( ImageFileFormat_Png, filename, ptrGrabResult);
            } else {
                cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }

        n=0;
        while ( lower.IsGrabbing() ) {
            lower.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
            if ( ptrGrabResult->GrabSucceeded()) {
                snprintf(filename, 100, "images/Lower%03d.png", n++);
                cout << "Writing image to " << filename << endl;
                // Convert to OpenCV Mat
		fc.Convert(image, ptrGrabResult);
		Mat cimg(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(),
			CV_8UC3, (uint8_t *)image.GetBuffer());
		imwrite(filename, cimg);
                //CImagePersistence::Save( ImageFileFormat_Png,  filename, ptrGrabResult);
            } else {
                cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }

        upper.Close();
        lower.Close();

    }
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }



    // Releases all pylon resources.
    PylonTerminate();

    return exitCode;
}
