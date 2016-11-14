#include <pylon/PylonIncludes.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

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
        CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
        camera.Open();

        cout << "Opened." << endl;

        camera.StartGrabbing(1);

        cout << "Started grabbing." << endl;

        while(camera.IsGrabbing()) {
            camera.WaitForFrameTriggerReady( 100, TimeoutHandling_ThrowException);
            camera.ExecuteSoftwareTrigger();
            cout << "Triggered." << endl;
            camera.RetrieveResult(500, ptrGrabResult, TimeoutHandling_ThrowException);
            if (ptrGrabResult->GrabSucceeded())
            {
                cout << "Got result." << endl;
                CPylonImage image;
                image.AttachGrabResultBuffer( ptrGrabResult);
                cout << "Got image." << endl;
                image.Save(ImageFileFormat_Png, "images/sample.png");
                cout << "Saved." << endl;
                image.Release();
                cout << "Released." << endl;
            }

        }

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
