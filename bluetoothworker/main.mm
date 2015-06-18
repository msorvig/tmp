
#include <QtCore>
#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <IOBluetooth/objc/IOBluetoothDeviceInquiry.h>

CFRunLoopRef cfloop = 0;

class BluetoothThread : public QThread
{
public:
    void run() {
        qDebug() << "Running thread" << QThread::currentThread();

        // start the run loop
        NSRunLoop* loop = [NSRunLoop currentRunLoop];
        cfloop = [loop getCFRunLoop];

        // NSRunLoop will exit "if no input sources or timers are attached to the run loop"
        // ### keep running it
        while (true) {
            [loop run];
        }
        
        qDebug() << "Thread exit!";
    }
};


@interface OSXBTDeviceInquiry : NSObject<IOBluetoothDeviceInquiryDelegate>
{

}
- (void)deviceInquiryComplete:(IOBluetoothDeviceInquiry *)sender
        error:(IOReturn)error aborted:(BOOL)aborted;

- (void)deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry *)sender
        device:(IOBluetoothDevice *)device;

- (void)deviceInquiryStarted:(IOBluetoothDeviceInquiry *)sender;
@end

@implementation OSXBTDeviceInquiry
- (void)deviceInquiryComplete:(IOBluetoothDeviceInquiry *)sender
        error:(IOReturn)error aborted:(BOOL)aborted
{
    Q_UNUSED(aborted)

}

- (void)deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry *)sender
        device:(IOBluetoothDevice *)device
{
}

- (void)deviceInquiryStarted:(IOBluetoothDeviceInquiry *)sender
{
    qDebug() << "IOBluetoothDeviceInquiry: started" << QThread::currentThread();
}

@end

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    
    BluetoothThread btt;
    btt.start();
    QThread::sleep(1); // ###
    qDebug() << "thread ready";
    
    // Run the bluetooth code on a worker thread with a run loop.
    CFRunLoopPerformBlock(cfloop, kCFRunLoopCommonModes, ^{
        qDebug() << "Running on thread" <<  QThread::currentThread();

        OSXBTDeviceInquiry *delegate = [[OSXBTDeviceInquiry alloc] init];
        IOBluetoothDeviceInquiry *inquiry = [[IOBluetoothDeviceInquiry inquiryWithDelegate:delegate] retain];
        [inquiry start];
    });

    return app.exec();
}
