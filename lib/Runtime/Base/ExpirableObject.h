//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
class ThreadContext;

class ExpirableObject: public FinalizableObject
{
public:
    ExpirableObject(ThreadContext* threadContext);

    virtual void Finalize(bool isShutdown);

    virtual void Dispose(bool isShutdown) override;

    virtual void Mark(Recycler *recycler) override { AssertMsg(false, "Mark called on object that isn't TrackableObject"); }

    // Called when an expirable object gets expired
    virtual void Expire() = 0;
    virtual void EnterExpirableCollectMode();

    bool IsObjectUsed();
    void SetIsObjectUsed();
    bool SupportsExpiration()
    {
        return (GetRegistrationHandle() != nullptr);
    }

    // Used by ThreadContext
    void * GetRegistrationHandle();
    void SetRegistrationHandle(void * registrationHandle);
    void ClearRegistrationHandle();
private:
    Field(intptr_t) registrationHandle;
};
