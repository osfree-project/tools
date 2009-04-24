unit abi;

interface


var
  rootpath: string;

type
  TAbiType=(AbiDynamicLibrary, AbiStaticLibrary, AbiInterrupt);

type
  TABI=record
    Name: ShortString;
    CallingConvertion: ShortString;
    case AbiType: TAbiType of
    AbiDynamicLibrary:
    (
      DLL: ShortString;
      ImportLibrary: ShortString;
      Ordinal: ShortString;
      ImportName: ShortString;
    );
    AbiStaticLibrary:
    (
      LibraryName: ShortString;
    );
    AbiInterrupt:
    (
      InterruptNumber: ShortString;
      Argument1: ShortString;
      Argument2: ShortString;
      Argument3: ShortString;
      Argument4: ShortString;
      Argument5: ShortString;
      Argument6: ShortString;
      Argument7: ShortString;
      Argument8: ShortString;
      Argument9: ShortString;
      Argument10: ShortString;
      Argument11: ShortString;
      Argument12: ShortString;
      Argument13: ShortString;
      Argument14: ShortString;
      Argument15: ShortString;
    )
  end;

function AbiGet(AbiFile: String; SymbolName: String): TABI;

implementation

function AbiGet(AbiFile: String; SymbolName: String): TABI;
var
  F: Text;
  S: String;
  T: String;
begin
  Assign(F, RootPath+AbiFile);
  Reset(F);
  While not EOF(F) do
  begin
    ReadLn(F, S);
    Result.Name:=Copy(S, 1, Pos(' ', S)-1);
    Delete(S, 1, Pos(' ', S));
    While Copy(S, 1, 1)=' ' do Delete(S, 1, 1);
    T:=Copy(S, 1, Pos(' ', S)-1);
    Delete(S, 1, Pos(' ', S));
    While Copy(S, 1, 1)=' ' do Delete(S, 1, 1);
    if T='DLL' then
    begin
      Result.AbiType:=AbiDynamicLibrary;
      Result.CallingConvertion:=Copy(S, 1, Pos(' ', S)-1);
      Delete(S, 1, Pos(' ', S));
      While Copy(S, 1, 1)=' ' do Delete(S, 1, 1);
      Result.DLL:=Copy(S, 1, Pos(' ', S)-1);
      Delete(S, 1, Pos(' ', S));
      While Copy(S, 1, 1)=' ' do Delete(S, 1, 1);
      Result.Ordinal:=Copy(S, 1, Pos(' ', S)-1);
      Delete(S, 1, Pos(' ', S));
      While Copy(S, 1, 1)=' ' do Delete(S, 1, 1);
      If pos(' ', S)>0 then
      begin
        Result.ImportLibrary:=Copy(S, 1, Pos(' ', S)-1);;
        Delete(S, 1, Pos(' ', S));
        While Copy(S, 1, 1)=' ' do Delete(S, 1, 1);
        Result.ImportName:=S;
      end else begin
        Result.ImportLibrary:=S;
        Result.ImportName:='';
      end;
    end;
    If Result.Name=SymbolName then break;
  end;
  If Result.Name<>SymbolName then
  begin
    Result.Name:='';
    Result.CallingConvertion:='';
    Result.DLL:='';
  end;
//  WriteLn('found ', result.name, ' for ', symbolname);
  Close(F);
end;

end.
