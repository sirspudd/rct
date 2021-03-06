#ifndef FinishMessage_h
#define FinishMessage_h

#include <rct/Message.h>
#include <rct/String.h>

class FinishMessage : public Message
{
public:
    enum { MessageId = FinishMessageId };

    FinishMessage(int status = 0)
        : Message(MessageId), mStatus(status)
    {
    }

    void perkele() { mStatus = -1; }
    int status() const { return mStatus; }

    virtual int encodedSize() const override { return sizeof(mStatus); }
    virtual void encode(Serializer &s) const override { s << mStatus; }
    virtual void decode(Deserializer &d) override { d >> mStatus; }
private:
    int mStatus;
};

#endif
