require File.expand_path(File.dirname(__FILE__) + '/spec_helper')
require "suffixarray"

describe "SuffixArray" do

  it "construction with no argument -> raise ArgumentError" do
    lambda { SuffixArray.new() }.should raise_exception(ArgumentError)
  end

  it "construction with string" do
    SuffixArray.new("hoge").should_not be_nil
    SuffixArray.new("hoge").should be_instance_of(SuffixArray)
  end

  it "#text method returns source text string" do
    sa = SuffixArray.new "abracadabra"
    sa.text.should == "abracadabra"
  end

  it "#ipoint method returns an array containing index points" do
    sa = SuffixArray.new "abracadabra"
    ip = sa.ipoint
    ip.should be_instance_of(Array)
    ip.should == [10, 7, 0, 3, 5, 8, 1, 4, 6, 9, 2]
  end
  
end
