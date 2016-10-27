#include "XdeltaPatch.h"
#include <node_buffer.h>

using namespace v8;
using namespace node;

XdeltaPatch::XdeltaPatch(int s, int d, Local<Object> cfg) : 
    XdeltaOp(s, d, cfg, eOpPatch)
{

}

XdeltaPatch::~XdeltaPatch()
{}

void XdeltaPatch::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);

  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("XdeltaPatch"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "patchChunked", PatchChunked);

  target->Set(String::NewSymbol("XdeltaPatch"), constructor_template->GetFunction());
}

Handle<Value> XdeltaPatch::New(const Arguments& args)
{
  HandleScope scope;

  if (args.Length() < 3 || !args[0]->IsInt32() || !args[1]->IsInt32() || !args[2]->IsObject())
    return ThrowException(Exception::TypeError(String::New("arguments are (fd, fd, obj)")));

  XdeltaPatch* aXD = new XdeltaPatch(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->ToObject());

  aXD->Wrap(args.This());

  return args.This();
}

Handle<Value> XdeltaPatch::PatchChunked(const Arguments& args)
{
  HandleScope scope;

  if (args.Length() < 1 || (args.Length() > 1 && !Buffer::HasInstance(args[0])) || !args[args.Length()-1]->IsFunction())
    return ThrowException(Exception::TypeError(String::New("arguments are ([buffer], function)")));

  XdeltaPatch* aXd = ObjectWrap::Unwrap<XdeltaPatch>(args.This());

  if (aXd->mBusy)
    return ThrowException(Exception::TypeError(String::New("object busy with async op")));

  if (aXd->mErrType != eErrNone || aXd->mState == eDone) {
    aXd->Callback(Local<Function>::Cast(args[args.Length()-1]));
    return args.This();
  }
    
  if (args.Length() == 1) {
    aXd->mBuffMaxSize = 0;
  } else {
    aXd->mBufferObj = Persistent<Object>::New(args[0]->ToObject());
    aXd->mBuffMaxSize = Buffer::Length(aXd->mBufferObj);
    aXd->mBuff = Buffer::Data(aXd->mBufferObj);
  }
  aXd->mBuffLen = aXd->mBuffMaxSize;

  aXd->StartAsync(Local<Function>::Cast(args[args.Length()-1]));

  return args.This();
}

void XdeltaPatch::FinishAsync() 
{
    XdeltaOp::FinishAsync();
    mBufferObj.Dispose();
}