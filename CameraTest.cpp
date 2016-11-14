#include <pylon/PylonIncludes.h>

#include "Samples/C++/include/ConfigurationEventPrinter.h"
#include "Samples/C++/include/ImageEventPrinter.h"

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

//Example of an image event handler.
class CImageSaver : public CImageEventHandler
{
public:
    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
    {
        cout << "Got result." << endl;
        if (ptrGrabResult->GrabSucceeded())
        {
            CPylonImage image;
            image.AttachGrabResultBuffer( ptrGrabResult);
            cout << "Got image." << endl;
            image.Save(ImageFileFormat_Png, "images/sample.png");
            cout << "Saved." << endl;
            image.Release();
            cout << "Released." << endl;
        }


    }
};

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;
    CGrabResultPtr ptrGrabResult;

    // Before using any pylon methods, the pylon runtime must be initialized.
    PylonInitialize();

    try
    {
        // Create an instant camera object with the camera device found first.
        // As soon as this object goes out of scope, the thread is killed and
        // no callbacks occur.
        CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
        camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete);
        camera.RegisterImageEventHandler( new CImageEventPrinter, RegistrationMode_Append, Cleanup_Delete);
        camera.RegisterImageEventHandler( new CImageSaver, RegistrationMode_Append, Cleanup_Delete);
        camera.Open();

        cout << "Opened." << endl;
        if (camera.CanWaitForFrameTriggerReady()) {
            camera.StartGrabbing( GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
            cout << "Started grabbing." << endl;
            if ( camera.WaitForFrameTriggerReady( 100, TimeoutHandling_ThrowException))
            {
                camera.ExecuteSoftwareTrigger();
                cout << "Triggered." << endl;
            }
        } else {
            cout << "Can't wait for frame trigger on this camera?" << endl;
        }

        cout << "Sleeping." << endl;
        WaitObject::Sleep( 3*1000);

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
