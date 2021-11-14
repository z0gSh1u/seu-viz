N = size(I);
N = N(1);

minI = min(min(min(I)));
maxI = max(max(max(I)));
range = maxI - minI;
I256 = uint16((I - minI) / range * 255);

% slice = squeeze(I256(30, :, :));
% imshow(slice, []);

fid = fopen(strcat('shep3d_', num2str(N), '.uint16.raw'), 'wb');
fwrite(fid, I256, 'uint16');
fclose(fid);
 
disp('Done.');